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
	#include "mbedtls/x509.h"
//	#include "mbedtls/platform.h"
	#include "mbedtls/error.h"
	#include "mbedtls/md.h"
	#include "mbedtls/pk.h"
	#include "mbedtls/entropy.h"
	#include "mbedtls/ctr_drbg.h"
	#include "mbedtls/ssl.h"

#define	SGNFAIL(__x__)		cerr << "## ERROR: " << AJAFUNC << ": " << __x__ << endl
#define	SGNWARN(__x__)		cerr << "## WARNING: " << AJAFUNC << ": " << __x__ << endl
#define	SGNNOTE(__x__)		cout << "## NOTE: " << AJAFUNC << ": " << __x__ << endl
#define	SGNINFO(__x__)		cout << "## INFO: " << AJAFUNC << ": " << __x__ << endl
#define	SGNDBG(__x__)		if (gIsVerbose) cout << AJAFUNC << ": " << __x__ << endl

using namespace std;


// Globals
static bool gGlobalQuit (false);  /// Set this "true" to exit gracefully
static int gIsVerbose (false);
static int gShowCertInfo (false);


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}

static string mbedErrStr (const int returnCode)
{
	static NTV2Buffer sErrBuff(4096);
	string str;
	mbedtls_strerror(returnCode, sErrBuff, sErrBuff);
	sErrBuff.GetString (str, /*U8Offset*/0, /*maxSize*/sErrBuff);
	return str;
}

class Path	//	A crude substitute for std::filesystem::path (C++17)
{
	public:
		Path (const string & inPath) : mPathDelim(1, AJA_PATHSEP), mElements(aja::split(inPath, mPathDelim))
			{	if (!mElements.empty() && mElements.back().empty())
					mElements.pop_back();
			}
		string	name (void) const	{return mElements.empty() ? string() : mElements.back();}
		NTV2StringList	nameParts (void) const	{return aja::split(name(),".");}
		string baseName (void) const	{NTV2StringList p(nameParts()); if (p.size() > 1) p.pop_back(); return aja::join(p,".");}
		string extension (void) const	{return nameParts().size() > 1 ? nameParts().back() : string();}
		string fullPath (void) const	{return aja::join(mElements, mPathDelim);}
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
		string			mPathDelim;
		NTV2StringList	mElements;
};	//	Path


static bool signItem (const string & inItemPath, const NTV2Buffer & inCert, mbedtls_pk_context & inKey)
{
	//	Compute .sig file path from item path...
	Path sigPath(inItemPath);
	sigPath.setExtension("sig");
	if (sigPath.fileExists()  && gIsVerbose)
		SGNWARN("'" << sigPath.fullPath() << "' exists -- will overwrite");

#if defined(MBEDTLS_USE_PSA_CRYPTO)
	psa_crypto_init();
#endif /* MBEDTLS_USE_PSA_CRYPTO */

	//	Seed the RNG (random number generator)...
	NTV2Buffer customStr(128);
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_entropy_init(&entropy);
	mbedtls_ctr_drbg_init(&ctr_drbg);
	int ret = mbedtls_ctr_drbg_seed (&ctr_drbg, mbedtls_entropy_func, &entropy, customStr, customStr);
	if (ret)
	{	SGNFAIL("'mbedtls_ctr_drbg_seed' returned " << ret << " (" << mbedErrStr(ret) << ")");
		mbedtls_ctr_drbg_free(&ctr_drbg);
		mbedtls_entropy_free(&entropy);
		return false;
	}

	//	Convert certificate to hex string, then add to dictionary...
	NTV2Dictionary dict;	//	Key/value pair dictionary (JSON later)
	string certHexStr;
	if (!inCert.toHexString(certHexStr))
		{SGNFAIL("'toHexString' failed for certificate"); return false;}
	dict.insert(kNTV2PluginSigFileKey_X509Certificate, certHexStr);	//	Add to dictionary

	//	Generate SHA-256 signature...
	NTV2Buffer hash(32);
	ret = mbedtls_md_file (mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), inItemPath.c_str(), hash);
	if (ret)
		{SGNFAIL("'mbedtls_md_file' returned " << ret << " (" << mbedErrStr(ret) << ") for '" << inItemPath << "'"); return false;}
	if (gIsVerbose)
		{string str;  if (hash.toHexString(str)) cout << "## DEBUG: Digest: " << str << endl;}

	//	Sign the item (using the 'mbedtls_ctr_drbg_random' RNG)...
	NTV2Buffer signature(MBEDTLS_PK_SIGNATURE_MAX_SIZE);
	size_t sigBytesWritten(0);
	ret = mbedtls_pk_sign (&inKey,					//	private key context
							MBEDTLS_MD_SHA256,		//	hash algorithm that was used
							hash,					//	hash of item to be signed
							0,						//	hash length		**MrBill**	WHAT DOES ZERO MEAN?!?!
							signature, signature,	//	signature buffer address & capacity
							&sigBytesWritten,		//	number of bytes written into signature buffer
							mbedtls_ctr_drbg_random, &ctr_drbg);	//	random number generator function & param
	if (ret)
	{
		SGNFAIL("'mbedtls_pk_sign' returned " << ret << " (" << mbedErrStr(ret) << ") for '" << inItemPath << "'");
		mbedtls_ctr_drbg_free(&ctr_drbg);
		mbedtls_entropy_free(&entropy);
		return false;
	}
	if (!signature.Truncate(sigBytesWritten))
	{	SGNFAIL("Signature truncation to " << sigBytesWritten << " failed for '" << inItemPath << "'");
		mbedtls_ctr_drbg_free(&ctr_drbg);
		mbedtls_entropy_free(&entropy);
		return false;
	}
	if (gIsVerbose)
		SGNNOTE("Successfully signed '" << inItemPath << "'");

	//	Convert signature to hex string, then add to dictionary...
	string sigHexStr;
	if (!signature.toHexString(sigHexStr))
		{SGNFAIL("'toHexString' failed for signature"); return false;}
	dict.insert(kNTV2PluginSigFileKey_Signature, sigHexStr);	//	Add to dictionary
	if (gIsVerbose)
		{cout << "## DEBUG: Signature: " << sigHexStr << endl;}

	//	Serialize dictionary and write it into .sig file...
	string infoStr;
	if (!dict.serialize(infoStr))
		{SGNFAIL("Unable to 'serialize': " << dict);  return false;}
	ofstream sigF (sigPath.fullPath().c_str(), std::ios::out | std::ios::binary);
	if (!sigF.good())
		{SGNFAIL("Cannot open '" << sigPath.fullPath() << "' for writing");  return false;}
	sigF.write(&infoStr[0], infoStr.size());
	if (!sigF.good())
		{SGNFAIL("Failed to write '" << sigPath.fullPath() << "'");  return false;}
	if (gIsVerbose)
		SGNNOTE("Wrote signature file '" << sigPath.fullPath() << "'");
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
	return true;
}	//	signItem

static bool certIsValid (NTV2Buffer & outCertFileContents, const string & inCertPath)
{
	int ret(0);
	{	NTV2Buffer tmp(512*1024*1024);	//	no more than 500MB
		ifstream certF(inCertPath.c_str(), std::ios::in | std::ios::binary);
		if (!certF.good())
			{SGNFAIL("Certificate file '" << inCertPath << "' missing");  return false;}
		if (!certF.read(tmp, tmp.GetByteCount()).eof())
			{SGNWARN("EOF not reached in certificate file '" << inCertPath << "' -- over 500MB in size?");  return false;}
		tmp.Truncate(size_t(certF.gcount() + 1));	//	+1 because must include terminating NULL byte
		outCertFileContents = tmp;
	}

	mbedtls_x509_crt crt;
	mbedtls_x509_crt_init(&crt);
	ret = mbedtls_x509_crt_parse(&crt, outCertFileContents, outCertFileContents);
	if (ret)
	{	SGNFAIL("'mbedtls_x509_crt_parse' returned " << ret << " (" << mbedErrStr(ret) << ") for X509 cert found in '" << inCertPath << "'");
		mbedtls_x509_crt_free(&crt);
		return false;
	}
	//	TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD
	//	Validate the cert was created with (signed by) AJA's CA cert...
	//	TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD TBD
	if (gShowCertInfo)
	{
		NTV2Buffer msgBuff(4096);
		int msgLength (mbedtls_x509_crt_info (msgBuff, msgBuff, /*prefixString*/"", &crt));
		string msg (msgBuff, size_t(msgLength));
		if (!msg.empty())
			SGNINFO("'mbedtls_x509_crt_info' returned certificate info from '" << inCertPath << "':\n" << msg);
	}
	mbedtls_x509_crt_free(&crt);
	return true;
}

static bool keyIsValid (mbedtls_pk_context & outPvtKey, const string & inKeyPath, const string & inKeyPwd)
{
	int ret(0);
	mbedtls_pk_init(&outPvtKey);
	ret = mbedtls_pk_parse_keyfile (&outPvtKey, inKeyPath.c_str(), /*password*/inKeyPwd.c_str(), /*RNGfunction*/AJA_NULL, /*RNGparam*/AJA_NULL);
	if (ret)
	{	SGNFAIL("'mbedtls_pk_parse_keyfile' returned " << ret << " (" << mbedErrStr(ret) << ")");
		mbedtls_pk_free(&outPvtKey);
		return false;
	}
	return true;
}

int main (int argc, const char ** argv)
{
	char	*pCertPath(AJA_NULL), *pKeyPath(AJA_NULL), *pKeyPass(AJA_NULL);
	int		showVersion(0);
	poptContext	optionsContext;	//	Context for parsing command line arguments

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"version",	0,		POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",			AJA_NULL},
		{"cert",	'c',	POPT_ARG_STRING,	&pCertPath,		0,	"signing certificate to use",	"path to cert file"},
		{"key",		'k',	POPT_ARG_STRING,	&pKeyPath,		0,	"private key to sign with",		"path to key file"},
		{"password",'p',	POPT_ARG_STRING,	&pKeyPass,		0,	"key file password",			AJA_NULL},
		{"verbose",	'v',	POPT_ARG_NONE,		&gIsVerbose,	0,	"verbose mode?",				AJA_NULL},
		{"info",	0,		POPT_ARG_NONE,		&gShowCertInfo,	0,	"show certificate info?",		AJA_NULL},
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

	const string keyPath (pKeyPath ? pKeyPath : "");
	if (keyPath.empty())
		{cerr << "## ERROR: No private signing key path specified -- use '--key' option" << endl;  return 2;}

    NTV2Buffer certFileContents;
	if (!certIsValid(certFileContents, certPath))
		return 3;

    mbedtls_pk_context pvtKey;
	const string keyPass (pKeyPass ? pKeyPass : "");
	if (!keyIsValid(pvtKey, keyPath, keyPass))
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
		else if (signItem(itemPath, certFileContents, pvtKey))
			signedItems.insert(itemPath);
		else
			signFailures++;
	}
	mbedtls_pk_free(&pvtKey);

	return 0;
}	//	main
