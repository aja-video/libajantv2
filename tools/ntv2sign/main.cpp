/* SPDX-License-Identifier: MIT */
/**
	@file		crossplatform/ntv2sign/main.cpp
	@brief		Command line tool to sign NTV2 plugins.
	@copyright	(C) 2023-2024 AJA Video Systems, Inc.  All rights reserved.
**/

#include <csignal>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "ntv2utils.h"
#include "ntv2nubaccess.h"
#include "ajabase/common/common.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/system/info.h"
#include "ajabase/system/file_io.h"

using namespace std;


// Globals
static bool gGlobalQuit (false);  /// Set this "true" to exit gracefully
static int gIsVerbose (false);
static string gDelim (1, AJA_PATHSEP);


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}

class Path	//	A very crude substitute for std::filesystem::path, but also can read file contents
{
	public:
		Path (const string & inPath) : mElements(aja::split(inPath, gDelim))
			{	if (!mElements.empty() && mElements.back().empty())
					mElements.pop_back();
			}
		string	name (void) const	{return mElements.empty() ? string() : mElements.back();}
		NTV2StringList	nameParts (void) const	{return aja::split(name(),".");}
		string baseName (void) const	{NTV2StringList p(nameParts()); if (p.size() > 1) p.pop_back(); return aja::join(p,".");}
		string extension (void) const	{return nameParts().size() > 1 ? nameParts().back() : string();}
		string fullPath (void) const	{return aja::join(mElements,gDelim);}
		bool exists (void) const		{return fileExists() || folderExists();}
		bool folderExists (void) const	{return AJAFileIO::DirectoryExists(fullPath());}
		bool fileExists (void) const	{return AJAFileIO::FileExists(fullPath());}
		size_t fileSize (void) const
			{	AJAFileIO fio;  int64_t cre(0), mod(0), sz(0);
				if (!fileExists())
					return 0;
				if (AJA_FAILURE(fio.Open(fullPath(), eAJAReadOnly, eAJABuffered)) )
					return 0;
				return AJA_SUCCESS(fio.FileInfo(cre, mod, sz)) ? size_t(sz) : 0;
			}
		bool readFile (NTV2Buffer & outData) const
			{
				if (!fileExists())
					return false;
				if (!outData.Allocate(fileSize()))
					return false;
				AJAFileIO fio;
				if (AJA_FAILURE(fio.Open(fullPath(), eAJAReadOnly, eAJABuffered)) )
					return false;
				if (fio.Read(outData, outData.GetByteCount()) < outData.GetByteCount())
					return false;
				return true;
			}
		bool writeFile (const NTV2Buffer & inData) const
			{
				if (inData.IsNULL())
					return false;
				AJAFileIO fio;
				if (AJA_FAILURE(fio.Open(fullPath(), eAJAWriteOnly | (fileExists() ? 0 : eAJACreateNew), eAJABuffered)) )
					return false;
				if (fio.Write(inData, inData.GetByteCount()) < inData.GetByteCount())
					return false;
				fio.Sync();
				fio.Close();
				return true;
			}
		bool setExtension (const string & inNewExt)
			{	NTV2StringList p(nameParts());
				if (p.size() > 1)
					p.pop_back();
				p.push_back(inNewExt);
				mElements.pop_back();
				mElements.push_back(aja::join(p,"."));
				return true;
			}
	private:
		NTV2StringList	mElements;
};	//	Path

static bool signItem (const string & inItemPath)
{
	Path itemPath(inItemPath), sigPath(itemPath);
	if (itemPath.folderExists())
		{cerr << "## ERROR:  '" << inItemPath << "' is a directory -- cannot sign" << endl;  return false;}
	if (!itemPath.fileExists())
		{cerr << "## ERROR:  '" << inItemPath << "' not found" << endl;  return false;}
	NTV2Buffer itemContents;
	if (!itemPath.readFile(itemContents))
		{cerr << "## ERROR:  Cannot read '" << inItemPath << "' contents" << endl;  return false;}
	sigPath.setExtension("sig");
	if (sigPath.fileExists())
		cerr << "## WARNING:  '" << sigPath.fullPath() << "' exists -- will overwrite" << endl;

	//	Make the key/value pair dictionary (maybe JSON later)...
	NTV2Dictionary dict;
	string infoStr;
	dict.insert(kNTV2PluginRegInfoKey_Vendor, "AJA Video Systems, Inc.");
	dict.insert(kNTV2PluginRegInfoKey_CommonName, "aja.com");
	dict.insert(kNTV2PluginRegInfoKey_LongName, "Software Device");
	dict.insert(kNTV2PluginRegInfoKey_ShortName, "swdevice");
	dict.insert(kNTV2PluginRegInfoKey_Description, "Implements an NTV2 device in software");
	dict.insert(kNTV2PluginRegInfoKey_Copyright, "Copyright (C) 2024 AJA Video Systems");
	NTV2Buffer phonySig(4096);  for (int ndx(0); ndx < 4096; ndx++) phonySig.U8(ndx) = uint8_t(ndx);
	string hexStr, fakeFingerprint("1A2B3C4D5E6F1A2B3C4D5E6F1A2B3C4D5E6F1A2B3C4D5E6F6F6F6F6F6F6F6F6F");
	phonySig.toHexString(hexStr);
	dict.insert("DigitalSignature", hexStr);
	dict.insert("Fingerprint", fakeFingerprint);
	dict.toString(infoStr);
	//	Make the .sig file...
	NTV2Buffer sigInfo(infoStr.size());
	sigInfo.CopyFrom(&infoStr[0], infoStr.size());
	if (!sigPath.writeFile(sigInfo))
		{cerr << "## ERROR:  Cannot write '" << sigPath.fullPath() << "' contents" << endl;  return false;}
	if (gIsVerbose)
		cout << "## NOTE:  Signed '" << inItemPath << "', wrote signature file '" << sigPath.fullPath() << "'" << endl;
	return true;
}

static bool certIsValid (const string & inCertPath)
{
	Path certPath(inCertPath);
	if (certPath.folderExists())
		{cerr << "## ERROR:  '" << inCertPath << "' is a directory, not a certificate" << endl;  return false;}
	if (!certPath.fileExists())
		{cerr << "## ERROR:  '" << inCertPath << "' not found" << endl;  return false;}
	NTV2Buffer certContents;
	if (!certPath.readFile(certContents))
		{cerr << "## ERROR:  Cannot read certificate '" << inCertPath << "' contents" << endl;  return false;}

	//	TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD
	//	Validate the cert was created with (signed by) AJA's CA cert...
	//	TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD

	if (gIsVerbose)
		cout << "## NOTE:  Checking certificate file '" << inCertPath << "'" << endl;
	return true;
}

int main (int argc, const char ** argv)
{
	char *	pCertPath(AJA_NULL);
	int		showVersion(0);
	poptContext	optionsContext;	//	Context for parsing command line arguments

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"version",	0,		POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",			AJA_NULL},
		{"cert",	'c',	POPT_ARG_STRING,	&pCertPath,		0,	"signing certificate to use",	"path to .cer file"},
		{"verbose",	'v',	POPT_ARG_NONE,		&gIsVerbose,	0,	"verbose mode?",				AJA_NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	NTV2StringList itemsToBeSigned;
	NTV2StringSet signedItems;
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	int res(::poptGetNextOpt(optionsContext));
	if (res < -1)
		{cerr << "## ERROR: " << ::poptBadOption(optionsContext,0) << ": " << ::poptStrerror(res) << endl;  return 2;}
	else
	{
		const char * pStr (::poptGetArg(optionsContext));
		while (pStr)
		{
			itemsToBeSigned.push_back(string(pStr));
			pStr = ::poptGetArg(optionsContext);
		}
	}
	::poptFreeContext(optionsContext);
	if (showVersion)
		{cout << argv[0] << ", NTV2 SDK " << ::NTV2Version() << endl;  return 0;}

	if (itemsToBeSigned.empty())
		{cerr << "## ERROR: No file(s) specified to be signed" << endl;  return 2;}

	const string certPath (pCertPath ? pCertPath : "");
	if (certPath.empty())
		{cerr << "## ERROR: No certificate file path specified -- use '--cert' option" << endl;  return 2;}

	if (!certIsValid(certPath))
		return 3;

	::signal (SIGINT, SignalHandler);
#if defined (AJAMac)
	::signal (SIGHUP, SignalHandler);
	::signal (SIGQUIT, SignalHandler);
#endif

	size_t signFailures(0);
	for (size_t ndx(0);  !signFailures  &&  ndx < itemsToBeSigned.size();  ndx++)
	{	const string & itemPath(itemsToBeSigned.at(ndx));
		if (signedItems.find(itemPath) != signedItems.end())
			cerr << "## WARNING: '" << itemPath << "' already signed" << endl;
		else if (signItem(itemPath))
			signedItems.insert(itemPath);
		else
			signFailures++;
	}

	return 0;
}	//	main
