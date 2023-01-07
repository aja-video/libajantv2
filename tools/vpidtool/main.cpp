/**
	@file		crossplatform/vpidtool/main.cpp
	@brief		Command-line tool that converts a 32-bit VPID value to a human-readable string.
	@copyright	(C) 2019-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajabase/common/options_popt.h"
#include "ajabase/common/common.h"
#include "ntv2utils.h"
#include "ntv2endian.h"
#include "ntv2vpid.h"
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;


/**
	@return		The given string converted to lower-case.
	@param[in]	str		Specifies the string to be converted to lower case.
**/
static inline string ToLower (string str)	{return aja::lower(str);}


/**
	@brief		Main entry point for 'vpidtool'.
	@param[in]	argc	Number arguments specified on the command line, including the path to the executable.
	@param[in]	argv	Array of 'const char' pointers, one for each argument.
	@return		Result code, which must be zero if successful, or non-zero for failure.
**/
int main (int argc, const char ** argv)
{
	bool			doSwap	(false);	//	Byte swap?
	char *			pFormat	(AJA_NULL);	//	What format is desired?
	poptContext		optionsContext;		//	Context for parsing command line arguments
	ULWordSequence	values;

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"format",	'f',	POPT_ARG_STRING,	&pFormat,	0,	"desired format",	"compact|table|json"},
//		{"swap",	's',	POPT_ARG_NONE,		&doSwap,	0,	"byte swap?",		AJA_NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	int res(::poptGetNextOpt (optionsContext));
	if (res < -1)
		{cerr << "## ERROR:  " << ::poptBadOption(optionsContext, 0) << ": " << ::poptStrerror(res) << endl;	return 1;}
	const char * pStr (::poptGetArg(optionsContext));
	while (pStr)
	{
		string	arg(ToLower(pStr));
		int	base(10);
		ULWord val(0);
		if (arg.find("0x") == 0)	{base = 16;	arg.erase(0,2);}
		else if (arg.find("x") == 0){base = 16; arg.erase(0,1);}
		val = ULWord(aja::stoul(arg, AJA_NULL, base));
		if (doSwap)
			val = NTV2EndianSwap32(val);
		values.push_back(val);
		//cerr << "'" << arg << "'\t" << DEC(val) << "\t" << xHEX0N(val,8) << endl;
		pStr = ::poptGetArg(optionsContext);
	}	//	for each file path argument
	optionsContext = ::poptFreeContext (optionsContext);
	if (values.empty())
		{cerr << "## ERROR:  No VPID values specified" << endl;  return 1;}

	const string format (pFormat ? ::ToLower(pFormat) : "table");
	if (format != "compact" && format != "c" && format != "table" && format != "t" && format != "json")
		{cerr << "## ERROR:  Bad '--format' value '" << format << "'" << endl;  return 1;}

	for (size_t ndx(0);  ndx < values.size();  ndx++)
	{
		const CNTV2VPID vpid(values.at(ndx));
		if (format == "json")
		{	AJALabelValuePairs info;
			vpid.GetInfo(info);
			cout	<< '{' << endl;
			for (AJALabelValuePairsConstIter it(info.begin());  it != info.end();  )
			{	string key(it->first), val(it->second);
				cout	<< "\t\"" << aja::replace(key, " ", "") << "\":\t\"" << val << '"';
				if (++it != info.end())
					cout << ",";
				cout << endl;
			}
			cout	<< '}' << ((ndx+1) == values.size() ? "" : ",") << endl;
		}
		else if (format == "table" || format == "t")
			cout << vpid.AsString(true) << endl;
		else
			cout << vpid << endl;
	}
	return 0;

}	//	main
