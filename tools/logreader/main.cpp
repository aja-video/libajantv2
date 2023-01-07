/* SPDX-License-Identifier: MIT */
/**
	@file		crossplatform/logreader/main.cpp
	@brief		Implements 'logreader' command.
	@copyright	(C) 2016-2022 AJA Video Systems, Inc.
**/
#include "ajantv2/includes/ntv2publicinterface.h"
#include "ajabase/common/common.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/system/debug.h"
#include <ajabase/system/process.h>
#include <ajabase/common/timer.h>
#include <ajabase/system/thread.h>
#include <ajabase/system/systemtime.h>
#include "ajabase/common/timebase.h"
#include <iostream>
#include <signal.h>
#include <string>

using namespace std;

#define	CONTAINS(__s__,__sub__)		((__s__).find(__sub__) != string::npos)
#define	STARTSWITH(__s__,__sub__)	((__s__).find(__sub__) == 0)


typedef	map<string,string>					StringMap;
typedef	StringMap::const_iterator			StringMapConstIter;

typedef set<AJADebugSeverity>				AJADebugSeverities;
typedef AJADebugSeverities::iterator		AJADebugSeveritiesIter;
typedef AJADebugSeverities::const_iterator	AJADebugSeveritiesConstIter;

typedef set<AJADebugUnit>					AJADebugUnits;
typedef	AJADebugUnits::iterator				AJADebugUnitsIter;
typedef	AJADebugUnits::const_iterator		AJADebugUnitsConstIter;


//	Custom formatting escape sequences:
static const string kEscIndexNumber	("%I");	//	%I	Index number
static const string kEscProcessID	("%p");	//	%p	PID
static const string kEscThreadID	("%t");	//	%t	TID
static const string kEscTimestamp	("%T");	//	%T	Timestamp
static const string kEscDebugUnit	("%D");	//	%D	Debug unit
static const string kEscSeverity	("%S");	//	%S	Severity
static const string kEscFullPath	("%P");	//	%P	Full source file path
static const string kEscFolderPath	("%F");	//	%F	Source folder path (part of full source file path)
static const string kEscFileName	("%N");	//	%N	Source file name (part of full source file path, includes extension)
static const string kEscBaseName	("%n");	//	%n	Source file base name (part of source file name, excludes extension)
static const string kEscFileExt		("%x");	//	%x	Source file name extension (part of file name)
static const string kEscLineNumber	("%L");	//	%L	Source file line number
static const string kEscMessage		("%M");	//	%M	Message text
static const string kEscPercent		("%%");	//	%%	Percent sign

static int		gIsVerbose(0);		//	Verbose output?
static int32_t	gRefCount(0);


static void SignalHandler (int inSignal)
{
	(void)inSignal;
	AJADebug::GetClientReferenceCount(&gRefCount);
	if (gIsVerbose)
		cerr << endl << "## NOTE: Closing, reference count is " << DEC(gRefCount) << endl;

	if (gRefCount > 0)
		AJADebug::SetClientReferenceCount(--gRefCount);
	AJADebug::Close();
	exit(1);
}


template<typename T> string NumToString (const T inValue)
{
	ostringstream	result;
	result << inValue;
	return result.str();
}


string & FormatPaths (string & outputString, const string & inFullPathStr)
{
	vector<string>	pathElements;
	string			fullPathStr(inFullPathStr), folderPath, pathDelim("\\");
	aja::replace(outputString, kEscFullPath, fullPathStr);
	if (fullPathStr.find('\\') != string::npos)
		aja::split(fullPathStr, '\\', pathElements);
	else if (fullPathStr.find('/') != string::npos)
		{aja::split(fullPathStr, '/', pathElements);	pathDelim = "/";}
	else
		pathElements.push_back(fullPathStr);

	const string	fileName(pathElements.back());
	pathElements.pop_back();
	folderPath = aja::join(pathElements, pathDelim);
	aja::replace(outputString, kEscFolderPath, folderPath);

	string	nameExtension, nameWithoutExtension;
	aja::replace(outputString, kEscFileName, fileName);
	if (fileName.find('.') != string::npos)
	{
		vector<string>	nameElements;
		aja::split(fileName, '.', nameElements);
		nameExtension = nameElements.back();
		nameElements.pop_back();
		nameWithoutExtension = aja::join(nameElements, ".");
	}
	aja::replace(outputString, kEscBaseName, nameWithoutExtension);
	aja::replace(outputString, kEscFileExt, nameExtension);
	return outputString;
}


class DebugInfoSettings
{
	//	Public Member Functions
	public:
		DebugInfoSettings()
		{
		    AJADebug::Open();
			for (AJADebugSeverity sv(AJA_DebugSeverity_Emergency);  sv < AJA_DebugSeverity_Size;  sv = AJADebugSeverity(sv+1))
			{	ostringstream	oss;	oss << DEC(sv);
				string	sevStr (AJADebug::GetSeverityString(sv));
				mSeverityToStr[sv] = sevStr;
				do
				{
					mStrToSeverity[sevStr] = sv;
					sevStr.resize(sevStr.length()-1);
					if (mStrToSeverity.find(sevStr) != mStrToSeverity.end())
						break;	//	Already exists
					if (sevStr.empty())
						break;	//	1 character minimum
				} while (true);
				mStrToSeverity[oss.str()] = sv;
			}
			for (AJADebugUnit du(AJA_DebugUnit_Unknown);  du < AJA_DebugUnit_Size;  du = AJADebugUnit(du+1))
			{	ostringstream	oss;	oss << DEC(du);
				string	duStr (AJADebug::GroupName(du));
				aja::replace(aja::replace(duStr, "AJA_DebugUnit_", ""), "unused_", "");
				mDebugUnitToStr[du] = duStr;
				mStrToDebugUnit[oss.str()] = du;
				if (oss.str() != duStr)
				{
					aja::lower(duStr);
					do
					{
						mStrToDebugUnit[duStr] = du;
						duStr.resize(duStr.length()-1);
						if (mStrToDebugUnit.find(duStr) != mStrToDebugUnit.end())
							break;	//	Already exists
						if (duStr.empty())
							break;	//	1 character minimum
						//cerr << DEC(du) << ":\t'" << duStr << "'" << endl;
					} while (true);
				}
			}
		}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	AJADebugSeverity Members
	public:
		const string &		SeverityToString (const AJADebugSeverity inSeverity) const
		{	static const string	sEmpty;
			SeverityToStringMapConstIter it(mSeverityToStr.find(inSeverity));
			if (it != mSeverityToStr.end())
				return it->second;
			return sEmpty;
		}
		inline bool			IsValidSeverityString (const string & inStr) const
		{
			return mStrToSeverity.find(inStr) != mStrToSeverity.end();
		}
		AJADebugSeverity	StringToSeverity (const string & inStr) const
		{
			StringToSeverityMapConstIter	it(mStrToSeverity.find(inStr));
			return it != mStrToSeverity.end() ? it->second : AJA_DebugSeverity_Size;
		}
		string				PrintLegalSeverities (void) const
		{	ostringstream oss;
			for (AJADebugSeverity sv(AJA_DebugSeverity_Debug);  ;  sv = AJADebugSeverity(sv-1))
			{
				oss << SeverityToString(sv);
				if (sv == AJA_DebugSeverity_Emergency)
					break;
				oss << "|";
			}
			return oss.str();
		}
		inline void			AddAllSeverities (void)
		{	mShowSeverities.clear();
			for (AJADebugSeverity sv(AJA_DebugSeverity_Emergency);  sv < AJA_DebugSeverity_Size;  sv = AJADebugSeverity(sv+1))
				mShowSeverities.insert(sv);
		}
		inline void			AddSeverity (const AJADebugSeverity inSeverity)
		{
			if (inSeverity >= AJA_DebugSeverity_Emergency  &&  inSeverity < AJA_DebugSeverity_Size)
				mShowSeverities.insert(inSeverity);
		}
		inline void			RemoveSeverity (const AJADebugSeverity inSeverity)
		{
			AJADebugSeveritiesIter it(mShowSeverities.find(inSeverity));
			if (it != mShowSeverities.end())
				mShowSeverities.erase(it);
		}
		inline bool			HasSeverity (const AJADebugSeverity inSeverity) const
		{
			return mShowSeverities.find(inSeverity) != mShowSeverities.end();
		}
		inline bool			HasAllSeverities (void) const		{return CountSeverities() == size_t(AJA_DebugSeverity_Size);}
		inline size_t		CountSeverities (void) const		{return mShowSeverities.size();}
		inline bool			ShowingNoSeverities (void) const	{return mShowSeverities.empty();}
		string				PrintSeverities (void) const
		{
			ostringstream oss;
			for (AJADebugSeveritiesConstIter it(mShowSeverities.begin());  it != mShowSeverities.end();  ++it)
				oss << "+" << SeverityToString(*it);
			return oss.str();
		}
		bool				ParseSeverities (const string & inArg, ostream & outWarnings)
		{
			string	severityStr(inArg), str;
			char addOrSub('+');
			unsigned	loopCount(0);
			if (severityStr.empty())
				{cerr << "## ERROR:  Empty severity argument" << endl;	return false;}
			char c(severityStr[0]);
			if (c == '*')
			{
				AddAllSeverities();
				severityStr.erase(0,1);	//	Eat it
			}
			while (!severityStr.empty())
			{
				c = severityStr[0];	severityStr.erase(0,1);	//	Eat it
				if (c == '+'  ||  c == '-')
				{
					if (!processSeverityString(str, addOrSub, outWarnings, !loopCount))
						return false;
					addOrSub = c;
				}
				else if ((c >= 'a' && c <= 'z')  ||  (c >= '0' && c <= '9'))
					str += c;
				else if (c == '?')
					{cerr	<< "## Legal severity values:  " << PrintLegalSeverities() << endl
							<< "## Precede with '+' to include, or '-' to exclude;  start with '*' to specify 'everything'" << endl
							<< "## EXAMPLE:  --severity=\"*-debug-info\"   Show all except Debug and Info messages" << endl
							<< "## EXAMPLE:  --severity=+notice+warn     Show only Notice and Warning messages" << endl;	return false;}
				else
					{cerr << "## ERROR: Invalid character '" << c << "' in severity expression '" << inArg << "'" << endl;	return false;}
				loopCount++;
			}
			return processSeverityString(str, addOrSub, outWarnings);
		}
	private:
		bool				processSeverityString (string & str, char & addOrSub, ostream & outWarnings, const bool ignoreEmpty = true)
		{
			if (!str.empty())
			{
				if (!IsValidSeverityString(str))
					{cerr << "## ERROR: Invalid severity specified '" << str << "' -- expected any of " << PrintLegalSeverities() << endl;	return false;}
				AJADebugSeverity sv(StringToSeverity(str));
				
				if (addOrSub == '+')
				{
					if (HasSeverity(sv))
					{
						outWarnings << "## WARNING:  Severity '" << str << "'";
						if (str != SeverityToString(sv))
							outWarnings << " (" << SeverityToString(sv) << ")";
						outWarnings << " already included" << endl;
					}
					AddSeverity(sv);
				}
				else
				{
					if (!HasSeverity(sv))
					{
						outWarnings << "## WARNING:  Severity '" << str << "'";
						if (str != SeverityToString(sv))
							outWarnings << " (" << SeverityToString(sv) << ")";
						outWarnings << " already excluded" << endl;
					}
					RemoveSeverity(sv);
				}
				str.clear();
			}
			else if (!ignoreEmpty)
				{cerr << "## ERROR: Missing severity value after '+' or '-'" << endl;	return false;}
			return true;
		}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	AJADebugUnit Members
	public:
		const string &		DebugUnitToString (const AJADebugUnit inDebugUnit) const
		{	static const string sUnknown("<unknown>");
			DebugUnitToStringMapConstIter it(mDebugUnitToStr.find(inDebugUnit));
			return it != mDebugUnitToStr.end() ? it->second : sUnknown;
		}
		inline bool			IsValidDebugUnitString (const string & inStr) const
		{
			return mStrToDebugUnit.find(inStr) != mStrToDebugUnit.end();
		}
		AJADebugUnit		StringToDebugUnit (const string & inStr) const
		{
			StringToDebugUnitMapConstIter	it(mStrToDebugUnit.find(inStr));
			return it != mStrToDebugUnit.end() ? it->second : AJA_DebugUnit_Size;
		}

		string				PrintLegalDebugUnits (void) const
		{	ostringstream oss;
			for (AJADebugUnit dbgUnit(AJA_DebugUnit_Unknown);  dbgUnit < AJA_DebugUnit_Size;  dbgUnit = AJADebugUnit(dbgUnit+1))
			{
				if (dbgUnit < AJA_DebugUnit_FirstUnused)
					oss << DebugUnitToString(dbgUnit);
				else
					oss << DEC(dbgUnit);
				if (dbgUnit < AJADebugUnit(AJA_DebugUnit_Size-1))
					oss << "|";
			}
			return oss.str();
		}
		inline void			AddAllDebugUnits (void)
		{	mShowDebugUnits.clear();
			for (AJADebugUnit dbgUnit(AJA_DebugUnit_Unknown);  dbgUnit < AJA_DebugUnit_Size;  dbgUnit = AJADebugUnit(dbgUnit+1))
				mShowDebugUnits.insert(dbgUnit);
		}
		inline void			AddDebugUnit (const AJADebugUnit inDebugUnit)
		{
			if (inDebugUnit >= AJA_DebugUnit_Unknown  &&  inDebugUnit < AJA_DebugUnit_Size)
				mShowDebugUnits.insert(inDebugUnit);
		}
		inline void			RemoveDebugUnit (const AJADebugUnit inDebugUnit)
		{
			AJADebugUnitsIter it(mShowDebugUnits.find(inDebugUnit));
			if (it != mShowDebugUnits.end())
				mShowDebugUnits.erase(it);
		}
		inline bool			HasDebugUnit (const AJADebugUnit inDebugUnit) const
		{
			return mShowDebugUnits.find(inDebugUnit) != mShowDebugUnits.end();
		}
		inline bool			HasAllDebugUnits (void) const		{return CountDebugUnits() == size_t(AJA_DebugUnit_Size);}
		inline size_t		CountDebugUnits (void) const		{return mShowDebugUnits.size();}
		inline bool			ShowingNoDebugUnits (void) const	{return mShowDebugUnits.empty();}
		string				PrintDebugUnits (void) const
		{
			ostringstream oss;
			for (AJADebugUnitsConstIter it(mShowDebugUnits.begin());  it != mShowDebugUnits.end();  ++it)
				oss << "+" << DebugUnitToString(*it);
			return oss.str();
		}

		bool				ParseDebugUnits (const string & inArg, ostream & outWarnings)
		{
			string	debugUnitStr(inArg), str;
			char addOrSub('+');
			unsigned	loopCount(0);
			if (debugUnitStr.empty())
				{cerr << "## ERROR:  Empty debug unit argument" << endl;	return false;}
			char c(debugUnitStr[0]);
			if (c == '*')
			{
				AddAllDebugUnits();
				debugUnitStr.erase(0,1);	//	Eat it
			}
			while (!debugUnitStr.empty())
			{
				c = debugUnitStr[0];	debugUnitStr.erase(0,1);	//	Eat it
				if (c == '+'  ||  c == '-')
				{
					if (!processDebugUnitString(str, addOrSub, outWarnings, !loopCount))
						return false;
					addOrSub = c;
				}
				else if ((c >= 'a' && c <= 'z')  ||  (c >= '0' && c <= '9'))
					str += c;
				else if (c == '?')
					{cerr	<< "## Legal debug units:  " << PrintLegalDebugUnits() << endl
							<< "## Precede with '+' to include, or '-' to exclude;  start with '*' to specify 'everything'" << endl
							<< "## EXAMPLE:  -u\"*-AncGeneric-CC608Decode\"       Show all except AncGeneric & CC608Decode messages" << endl
							<< "## EXAMPLE:  -u+DriverInterface+AutoCirculate   Show only DriverInterface & AutoCirculate messages" << endl;	return false;}
				else
					{cerr << "## ERROR: Invalid character '" << c << "' in debug unit expression '" << inArg << "'" << endl;	return false;}
				loopCount++;
			}
			return processDebugUnitString(str, addOrSub, outWarnings);
		}
	private:
		bool				processDebugUnitString (string & str, char & addOrSub, ostream & outWarnings, const bool ignoreEmpty = true)
		{
			if (!str.empty())
			{
				if (!IsValidDebugUnitString(str))
					{cerr << "## ERROR: Invalid debug unit specified '" << str << "'" << endl;	return false;}
				AJADebugUnit dbgUnit(StringToDebugUnit(str));
				
				if (addOrSub == '+')
				{
					if (HasDebugUnit(dbgUnit))
					{
						outWarnings << "## WARNING:  Debug unit '" << str << "'";
						if (str != DebugUnitToString(dbgUnit))
							outWarnings << " (" << DebugUnitToString(dbgUnit) << ")";
						outWarnings << " already included" << endl;
					}
					AddDebugUnit(dbgUnit);
				}
				else
				{
					if (!HasDebugUnit(dbgUnit))
					{
						outWarnings << "## WARNING:  Debug unit '" << str << "'";
						if (str != DebugUnitToString(dbgUnit))
							outWarnings << " (" << DebugUnitToString(dbgUnit) << ")";
						outWarnings << " already excluded" << endl;
					}
					RemoveDebugUnit(dbgUnit);
				}
				str.clear();
			}
			else if (!ignoreEmpty)
				{cerr << "## ERROR: Missing debug unit value after '+' or '-'" << endl;	return false;}
			return true;
		}

	//	Data Types
	private:
		typedef	map<AJADebugSeverity, string>			SeverityToStringMap;
		typedef SeverityToStringMap::const_iterator		SeverityToStringMapConstIter;

		typedef	map<string, AJADebugSeverity>			StringToSeverityMap;
		typedef	StringToSeverityMap::const_iterator		StringToSeverityMapConstIter;

		typedef	map<AJADebugUnit, string>				DebugUnitToStringMap;
		typedef	DebugUnitToStringMap::const_iterator	DebugUnitToStringMapConstIter;

		typedef	map<string, AJADebugUnit>				StringToDebugUnitMap;
		typedef	StringToDebugUnitMap::const_iterator	StringToDebugUnitMapConstIter;

	//	Member Data
	private:
		SeverityToStringMap		mSeverityToStr;
		StringToSeverityMap		mStrToSeverity;
		AJADebugSeverities		mShowSeverities;	//	Which severities to show
		DebugUnitToStringMap	mDebugUnitToStr;
		StringToDebugUnitMap	mStrToDebugUnit;
		AJADebugUnits			mShowDebugUnits;	//	Which debug units to show
};


int main(int argc, const char *argv[])
{
	int				samplesPerSec	(100);		//	Message log rate, messages per second (defaults to once per second)
	int				pidFilter		(0);		//	Filter: process ID (defaults to 0 == don't filter by pid)
	int				tidFilter		(0);		//	Filter: thread ID (defaults to 0 == don't filter by tid)
	int				showVersion		(0);		//	Show version?
	int				listStats		(0);		//	List stats?
	int				enableDebugUnits(0);		//	Set debug units? (If true, also commit selected DUs to AJADebug -- affects all message listeners)
	char *			pUnits			(AJA_NULL);	//	Message debug units (defaults to all)
	char *			pSeverity		(AJA_NULL);	//	Message severities (defaults to all)
	char *			pSevThreshold	(AJA_NULL);	//	Severity threshold above which output goes to stderr
	char *			pFormatStr		(AJA_NULL);	//	Custom output formatting string
	poptContext		optionsContext;				//	Context for parsing command line arguments
	StringMap		sEscapes;
	const string	DLIM("\t");
	DebugInfoSettings	dbgInfo;
	AJADebugSeverity	sevThreshold(AJA_DebugSeverity_Size);	//	Default: all msgs to stdout

	//	Initialize custom formatting escape sequences:
	sEscapes["\\n"] = "\n";		//	\n	LineFeed		0x0A
	sEscapes["\\f"] = "\f";		//	\f 	FormFeed		0x0C
	sEscapes["\\r"] = "\r";		//	\r	CarriageReturn	0x0D
	sEscapes["\\\\"] = "\\";	//	\\	Backslash		0x5C
	sEscapes["\\a"] = "\a";		//	\a	Bell			0x07
	sEscapes["\\b"] = "\b";		//	\b	Backspace		0x08
	sEscapes["\\t"] = "\t";		//	\t	HTab			0x09
	sEscapes["\\v"] = "\v";		//	\v	VTab			0x0B
	//	Future:	\nnn 	arbitrary octal value 	nnn
	//	Future:	\xnn 	arbitrary hex value 	nn

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"unit",		'u',	POPT_ARG_STRING,	&pUnits,			0,		"Debug unit filter",			"[*|?]{{+|-}{0-84}}[...]"					},
		{"severity",	's',	POPT_ARG_STRING,	&pSeverity,			0,		"Severity level filter",		"[*|?]{{+|-}{deb|inf|not|warn|err}}[...]"	},
		{"threshold",	't',	POPT_ARG_STRING,	&pSevThreshold,		8,		"Stderr severity threshold",	"debug|info|notice|warn|err|assert|alert|emerg"},
		{"pid",			0,		POPT_ARG_INT,		&pidFilter,			0,		"Process ID filter",			"process ID"},
		{"tid",			0,		POPT_ARG_INT,		&tidFilter,			0,		"Thread ID filter",				"thread ID"},
		{"enable",		0,		POPT_ARG_NONE,		&enableDebugUnits,	0,		"Enable debug units",			""},
		{"format",		'f',	POPT_ARG_STRING,	&pFormatStr,		0,		"Custom formatting",			"%I|%P|%T|%t|%D|%S|%F|%L|%M|%%"},
//		{"rate",		'r',	POPT_ARG_INT,		&samplesPerSec,		0,		"Polling rate",					"samples per second"					},
		{"verbose",		'v',	POPT_ARG_NONE,		&gIsVerbose,		0,		"Verbose output",				""},
		{"version",		'V',	POPT_ARG_NONE,		&showVersion,		0,		"Display version info",			""},
		{"stats",		0,		POPT_ARG_NONE,		&listStats,			0,		"List active stats",			""},
		POPT_AUTOHELP
		POPT_TABLEEND
	};


	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	if (::poptGetNextOpt (optionsContext) < -1)
		{cerr << "## ERROR:  Bad command line argument(s)" << endl;		return 1;}
	optionsContext = ::poptFreeContext (optionsContext);

	//	Configure the reader based on cmd line args...
    string	severityStr (pSeverity ? pSeverity : "*");				aja::lower(severityStr);
    string	sevThresholdStr (pSevThreshold ? pSevThreshold : "");	aja::lower(sevThresholdStr);
	string	debugUnitsStr (pUnits ? pUnits : "*");					aja::lower(debugUnitsStr);
	const uint64_t	filterPID (static_cast<uint64_t>(pidFilter));
	const uint64_t	filterTID (static_cast<uint64_t>(tidFilter));
	string	formatStr (pFormatStr ? pFormatStr : "");
	for (StringMapConstIter it(sEscapes.begin());  it != sEscapes.end();  ++it)
		aja::replace(formatStr, it->first, it->second);

	//	More argument parsing...
	ostringstream err;
	if (!dbgInfo.ParseSeverities(severityStr, err))
		return 2;
	if (!dbgInfo.ParseDebugUnits(debugUnitsStr, err))
		return 2;
	if (!sevThresholdStr.empty())
	{
		if (!dbgInfo.IsValidSeverityString(sevThresholdStr))
			{cerr << "## ERROR:  Invalid threshold argument '" << sevThresholdStr << "'" << endl;	return 1;}
		sevThreshold = dbgInfo.StringToSeverity(sevThresholdStr);
	}
	if (!err.str().empty())
		cerr << err.str();

	if (!AJADebug::IsOpen())
		{cerr << "## ERROR: AJADebug facility not open" << endl;  return 1;}
	if (showVersion)
	{
		cout << (argv[0] ? argv[0] : "logreader") << ": AJADebug facility v" << DEC(AJADebug::Version()) << (AJADebug::IsDebugBuild() ? " (Debug)" : " (Release)") << endl
			<< "\t" << DEC(AJADebug::TotalBytes()) << "-byte system buffer" << endl
			<< "\t" << DEC(AJADebug::MessageRingCapacity()) << "-message capacity" << endl;
		if (AJADebug::HasStats())
			cout << "\t" << DEC(AJADebug::StatsCapacity()) << "-stat measurement capacity" << endl;
		else
			cout << "\tNo stat measurement support (requires " << DEC(sizeof(AJADebugShare)) << "-byte system buffer)" << endl;
		return 0;
	}
	if (listStats && !AJADebug::HasStats())
		{cerr << "## WARNING: 'stats' option specified, but stats not supported" << endl;  return 0;}
	else if (listStats)
	{
		uint32_t seqNum(0);
		vector<uint32_t>	statKeys;
		AJAStatus st(AJADebug::StatGetKeys(statKeys, seqNum));
		if (AJA_FAILURE(st))
			{cerr << "## ERROR: StatGetKeys failed, " << AJAStatusToString(st) << endl; return 3;}
		cout << DEC(statKeys.size()) << " active stat(s) (last mod seq " << DEC(seqNum) << "):" << endl;
		for (size_t num(0);  num < statKeys.size();  num++)
			cout << " " << DEC(statKeys.at(num));
		if (statKeys.empty())
			cout << " (none)" << endl;
		uint32_t available(AJADebug::StatsCapacity() - uint32_t(statKeys.size()));
		if (available)
			cout << DEC(available) << " free/available" << endl;
		return 0;
	}

	//	Report what will be shown...
	if (dbgInfo.ShowingNoSeverities())
		{cerr << "## WARNING: No severities specified -- no messages to show -- exiting" << endl;	return 0;}
	if (gIsVerbose)
	{
		if (!dbgInfo.HasAllSeverities())
			cerr << "## NOTE: Filtering: Showing messages having " << (dbgInfo.CountSeverities() == 1 ? "severity " : "severities ")
				<< dbgInfo.PrintSeverities() << endl;
		if (!dbgInfo.HasAllDebugUnits())
			cerr << "## NOTE: Filtering: Showing " << DEC(dbgInfo.CountDebugUnits()) << " debug unit(s): "
				<< dbgInfo.PrintDebugUnits() << endl;
		if (filterPID)
			cerr << "## NOTE: Filtering: Showing messages only from process " << DEC(filterPID) << endl;
		if (filterTID)
			cerr << "## NOTE: Filtering: Showing messages only from thread " << DEC(filterTID) << endl;
		if (samplesPerSec)
			cerr << "## NOTE: Will poll for messages " << DEC(samplesPerSec) << " times per second" << endl;
		if (sevThreshold == AJA_DebugSeverity_Size)
			cerr << "## NOTE: All messages will be written to stdout" << endl;
		else
			cerr << "## NOTE: Messages higher than severity '" << dbgInfo.SeverityToString(sevThreshold) << "' will be written to stderr; all others to stdout" << endl;
		if (!formatStr.empty())
			cerr << "## NOTE: Using custom message formatting" << endl;
	}
	if (enableDebugUnits)
	{
		for (AJADebugUnit du(AJA_DebugUnit_Unknown);  du < AJA_DebugUnit_Size;  du = AJADebugUnit(du+1))
			if (dbgInfo.HasDebugUnit(du))
				AJADebug::Enable(int32_t(du), AJA_DEBUG_DESTINATION_DEBUG);
			else
				AJADebug::Disable(int32_t(du), AJA_DEBUG_DESTINATION_DEBUG);
		if (gIsVerbose)
			cerr << "## NOTE: Selected debug unit(s) AJADebug::Enable'd -- all others AJADebug::Disable'd" << endl;
	}

	AJADebug::GetClientReferenceCount(&gRefCount);
	if (gIsVerbose)
		cerr << "## NOTE: Will increment reference count " << DEC(gRefCount) << endl;
	AJADebug::SetClientReferenceCount(++gRefCount);

    ::signal (SIGINT, SignalHandler);
    #if defined (AJAMac)
        ::signal (SIGHUP, SignalHandler);
        ::signal (SIGQUIT, SignalHandler);
    #endif
	AJATimeBase	mTimeBase;
	double		mLastTime(-1.0);
	int64_t		mFirstTime(0);
	uint64_t	mLastIndex(0);
	uint64_t	mReadIndex(0);
	AJADebug::GetSequenceNumber(&mReadIndex);
	if (mReadIndex < 1)
		mReadIndex = 1;
	mTimeBase.SetTickRate(AJA_DEBUG_TICK_RATE);
	do
	{
		uint64_t newIndex(0);
	    uint64_t messageIndex(0);
		while (AJA_SUCCESS(AJADebug::GetSequenceNumber(&newIndex))
			 &&  newIndex > mLastIndex
			 &&  AJA_SUCCESS(AJADebug::GetMessageSequenceNumber(mReadIndex, &messageIndex))
			 &&  messageIndex > mLastIndex)
		{
	        uint32_t destination;
	        mLastIndex = messageIndex;
			if (AJA_SUCCESS(AJADebug::GetMessageDestination(mReadIndex, &destination))  &&  (destination != AJA_DEBUG_DESTINATION_NONE))
			{
		        int32_t groupIndex(0);
				uint64_t time(0);
		        int32_t severity(0);
				const char* pMessage(AJA_NULL);
				if (AJA_SUCCESS(AJADebug::GetMessageGroup(mReadIndex, &groupIndex))
					&&  AJA_SUCCESS(AJADebug::GetMessageTime(mReadIndex, &time))
					&&  AJA_SUCCESS(AJADebug::GetMessageSeverity(mReadIndex, &severity))
					&&  AJA_SUCCESS(AJADebug::GetMessageText(mReadIndex, &pMessage)))
				{
					uint64_t pid(0);
					uint64_t tid(0);
					const string	msg(pMessage?pMessage:"");
					if (!mFirstTime)
						mFirstTime = int64_t(time);
					const double currentTime (double(mTimeBase.MicrosecondsToSeconds(int64_t(time) - mFirstTime)));
					if (mLastTime < 0)
						mLastTime = currentTime;
					AJADebug::GetProcessId(mReadIndex, &pid);
					AJADebug::GetThreadId(mReadIndex,  &tid);
					if ((!filterPID || (pid == filterPID))
						&&  (!filterTID || (tid == filterTID))
						&&  dbgInfo.HasSeverity(AJADebugSeverity(severity))
						&&  dbgInfo.HasDebugUnit(AJADebugUnit(groupIndex)))
					{
						const string & severityStr (dbgInfo.SeverityToString(AJADebugSeverity(severity)));
						int32_t lineNum(0);
						AJADebug::GetMessageLineNumber(mReadIndex, &lineNum);
						const char *	pFileName(AJA_NULL);
						AJADebug::GetMessageFileName(mReadIndex, &pFileName);
						const string	path(pFileName ? pFileName : "");
						ostream &	outputStream (severity < sevThreshold ? cerr : cout);
						if (formatStr.empty())
							outputStream	<< DEC(messageIndex)
											<< DLIM << DEC(pid)
											<< DLIM << DEC(tid)
											<< DLIM << currentTime
											<< DLIM << dbgInfo.DebugUnitToString(AJADebugUnit(groupIndex))
											<< DLIM << severityStr
											<< DLIM << path
											<< DLIM << DEC(lineNum)
											<< DLIM << msg
											<< endl;
						else
						{	//	Custom formatting:
							string	outputString (formatStr);
							aja::replace(outputString, kEscIndexNumber, NumToString(messageIndex));
							aja::replace(outputString, kEscProcessID, NumToString(pid));
							aja::replace(outputString, kEscThreadID, NumToString(tid));
							aja::replace(outputString, kEscTimestamp, NumToString(currentTime));
							aja::replace(outputString, kEscDebugUnit, dbgInfo.DebugUnitToString(AJADebugUnit(groupIndex)));
							aja::replace(outputString, kEscSeverity, severityStr);
							aja::replace(outputString, kEscLineNumber, NumToString(lineNum));
							aja::replace(outputString, kEscMessage, msg);
							aja::replace(outputString, kEscPercent, "%");
							FormatPaths(outputString, path);
							outputStream	<< outputString;	//	User responsible for linebreaks!
						}
					}	//	if not filtered out
					mLastTime = currentTime;
				}	//	if  GetMessageGroup|GetMessageTime|GetMessageSeverity|GetMessageText OK
			}	//	if MessageDestination != "none"
			mReadIndex++;	//	On to the next message
		} // while GetSequenceNumber OK
		if (samplesPerSec)
			AJATime::SleepInMicroseconds(1000000 / uint32_t(samplesPerSec));
	} while (true);	//	Loop til ctrl-c

}	//	main
