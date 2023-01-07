/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2firmwareinstaller/main.cpp
	@brief		Implements 'ntv2firmwareinstaller' command.
	@copyright	(C) 2014-2022 AJA Video Systems, Inc.	All rights reserved worldwide.
**/
#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2utils.h"
#include "ntv2bitfile.h"
#include "ntv2mcsfile.h"
#include "ntv2registers2022.h"
#include "sys/stat.h"
#include "ntv2democommon.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/system/debug.h"
#include "ajabase/system/systemtime.h"
#include "ntv2firmwareinstallerthread.h"
#include "ntv2konaflashprogram.h"
#include <iostream>
#include <iomanip>

using namespace std;


typedef list <string>				StringList;
typedef StringList::const_iterator	StringListConstIter;

#define	NTV2_SHOW_MAC_HEX(__mac__)	Hex0N(uint16_t(__mac__[0]),2) << ":"	<< Hex0N(uint16_t(__mac__[1]),2) << ":"	<< Hex0N(uint16_t(__mac__[2]),2) << ":"		\
									<< Hex0N(uint16_t(__mac__[3]),2) << ":"	<< Hex0N(uint16_t(__mac__[4]),2) << ":"	<< Hex0N(uint16_t(__mac__[5]),2)


static void ReportDeviceFlashStatus (CNTV2Card & inDevice)
{
	string	dateString, timeString, serialString, serialNumber;
	CNTV2KonaFlashProgram konaFlasher(inDevice.GetIndexNumber());
	ostringstream	warnings, notes;

	//	Running firmware checks...
	UWord	fwRev(0);
	string	runningBuildDate, runningBuildTime, installedBuildDate, installedBuildTime;
	inDevice.GetRunningFirmwareRevision(fwRev);
	inDevice.GetRunningFirmwareDate(runningBuildDate, runningBuildTime);

	if (konaFlasher.ReadHeader(MAIN_FLASHBLOCK))
	{
		installedBuildDate = konaFlasher.GetDate();
		installedBuildTime = konaFlasher.GetTime();
	}
	else
		warnings << "## WARNING:  No main bitfile found" << endl;

	if (inDevice.IsIPDevice())
	{
		ULWord pkg;
		inDevice.GetRunningFirmwarePackageRevision(pkg);
		cout << " Running Package: " << pkg << endl;
	}

	cout << "  Running FW Rev: " << xHex0N(fwRev,2) << "(" << DEC(fwRev) << ")" << endl;
	if (!runningBuildDate.empty())
	{
		cout << " Running Bitfile: " << runningBuildDate << " ";
		if (!runningBuildTime.empty())
			cout << runningBuildTime;
		cout << endl;
	}
	if (!installedBuildDate.empty())
	{
		cout << "    Main Bitfile: '" << konaFlasher.GetDesignName() << "' " << installedBuildDate;
		if (!installedBuildTime.empty())
			cout << " " << installedBuildTime;
		bool isInstalledFWRunning(false);
		if (konaFlasher.IsInstalledFWRunning(isInstalledFWRunning, warnings)  &&  !isInstalledFWRunning)
			cout << "  <== NOT CURRENTLY RUNNING -- NEEDS POWER-CYCLE TO TAKE EFFECT";
		cout << endl;
	}

	if (konaFlasher.ReadHeader(FAILSAFE_FLASHBLOCK))
		cout << "Failsafe Bitfile: '" << konaFlasher.GetDesignName() << "' " << konaFlasher.GetDate() << " " << konaFlasher.GetTime() << endl;
	else
		warnings << "## WARNING:  No fail-safe bitfile found" << endl;

	if (konaFlasher.ReadInfoString())
		cout << "    Package Info: " << konaFlasher.GetMCSInfo().c_str() << endl;
	else if (inDevice.IsIPDevice())
		warnings << "## WARNING:  Unable to read package info" << endl;

	if (runningBuildDate.empty()  &&  !::NTV2DeviceCanReportRunningFirmwareDate(konaFlasher.GetDeviceID()))
		notes << "## NOTE:  This device cannot report its running firmware date/time" << endl;

	if (konaFlasher.GetSerialNumberString(serialNumber))
	{
		cout << "   Serial Number: '" << serialNumber << "'" << endl;
		//cout	<< "EEPROM shadow of serial num:  0x" << hex << setw (16) << setfill ('0') << ntv2Card.GetSerialNumber()
		//		<< ", '" << CNTV2Card::SerialNum64ToString (ntv2Card.GetSerialNumber()) << "'" << endl;
	}
	else
		warnings << "## WARNING:  Unable to read serial number" << endl;

	MacAddr mac1, mac2;
	if (konaFlasher.ReadMACAddresses(mac1, mac2))
		cout	<< "MAC1=" << NTV2_SHOW_MAC_HEX(mac1.mac) << " MAC2=" << NTV2_SHOW_MAC_HEX(mac2.mac) << endl;

	if (!warnings.str().empty())	//	Any warnings?
		cerr	<< warnings.str();	//	Spew 'em to stderr
	if (!notes.str().empty())		//	Any notes?
		cerr	<< notes.str();		//	Spew 'em to stderr
}


/**
	ntv2firmwareinstaller [-d|--device spec] [-p|--progress] [-w|--wait] [-q|--quiet]  [bitFilePath [...]]

	Installs firmware on a given device.

	...where...

	path/to/bitfile			Specifies an absolute or relative path to the firmware bitfile to be --info'd or installed.

	-d |--device spec		Specifies the target device to be flashed using an index number, serial number or model name
							(see CNTV2DeviceScanner::GetFirstDeviceFromArgument). If not specified, defaults to the first
							device found (i.e., the one using index number zero).

	-p | --progress			(Optional)  Show installation progress.

	-w | --wait				(Optional)  Prompts user to "press Enter key" when installation completes.

	-q | --quiet			(Optional)  Don't show status messages.

	-i | --info				(Optional)  Don't install, just show info about specified firmware bitfile(s).
**/
int main (int argc, const char** argv)
{
	char *			pDeviceSpec			(AJA_NULL);		//	Which device to use
	char *			pFirmwareLicense	(AJA_NULL);
	int				bQuiet				(0);
	int				bProgress			(0);
	int				bWaitForEnterKey	(0);
	int				bBitfileInfo		(0);
	int				bForce				(0);
	StringList		bitfilePaths;
	poptContext		optionsContext;
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device",					"index#, serial#, or model"	},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device",					"index#, serial#, or model"	},
		{"force",		'f',	POPT_ARG_NONE,		&bForce,			0,	"Warning: force the program",	AJA_NULL},
		{"license",     'l',	POPT_ARG_STRING,	&pFirmwareLicense,	0,	"install firmware license",		"license"},
		{"progress",	'p',	POPT_ARG_NONE,		&bProgress,			0,	"show installation progress",	AJA_NULL},
		{"quiet",		'q',	POPT_ARG_NONE,		&bQuiet,			0,	"quiet mode",					AJA_NULL},
		{"wait",		'w',	POPT_ARG_NONE,		&bWaitForEnterKey,	0,	"press Enter when finished?",	AJA_NULL},
		{"info",		'i',	POPT_ARG_NONE,		&bBitfileInfo,		0,	"just show bitfile info",		AJA_NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	{
		optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
		::poptGetNextOpt (optionsContext);
		const char *	sBitfilePath	(::poptGetArg (optionsContext));
		while (sBitfilePath)
		{
			bitfilePaths.push_back (sBitfilePath);
			sBitfilePath = ::poptGetArg (optionsContext);
		}
		optionsContext = ::poptFreeContext (optionsContext);
	}

	//	Get device info...
	const string	deviceSpecifier	(pDeviceSpec ? pDeviceSpec : "0");
	const string	license			(pFirmwareLicense ? pFirmwareLicense : "");
	CNTV2Card		device;
	CNTV2DeviceScanner::GetFirstDeviceFromArgument (deviceSpecifier, device);
	
	if (device.IsIPDevice())
	{
		CNTV2KonaFlashProgram konaFlasher(device.GetIndexNumber());
		if (bForce)
			konaFlasher.SetMBReset();
		konaFlasher.CheckAndFixMACs();
	}

	if (bBitfileInfo)
	{
		//	--info option --- just dump bitfile info to cout
		if (bProgress)
			cerr << "## NOTE:  '--progress' option irrelevant with '--info' option -- ignored" << endl;
		if (bWaitForEnterKey)
			cerr << "## NOTE:  '--wait' option irrelevant with '--info' option -- ignored" << endl;

		bool	hasMCS	(false);
		size_t	maxlen	(0);
		for (StringListConstIter iter(bitfilePaths.begin());  iter != bitfilePaths.end();  ++iter)
		{
			if (iter->length() > maxlen)
				maxlen = iter->length();
			if (iter->rfind(".mcs") != string::npos)
				hasMCS = true;
		}

		if (bitfilePaths.size() > 1 && !bQuiet)
			cout	<< bitfilePaths.size() << " bitfile paths specified:" << endl
					<< left << setw(int(maxlen)) << "Bitfile Path"      << "  " << left << "Device      Date       Time      Design            " << (hasMCS ? "  Pkg Version           Pkg Date  " : "") << endl
					<< left << setw(int(maxlen)) << string(maxlen, '-') << "  " << left << "----------- ---------- --------  ------------------" << (hasMCS ? "  --------------------  ----------" : "") << endl;
		for (StringListConstIter iter (bitfilePaths.begin());  iter != bitfilePaths.end();  ++iter)
		{
			const string &			bitfilePath (*iter);
			const string::size_type	posMCS		(bitfilePath.rfind(".mcs"));
			const string::size_type	posBIT		(bitfilePath.rfind(".bit"));

			if (posBIT != string::npos  &&  (posBIT + 4) == bitfilePath.length())
			{
				CNTV2Bitfile	bitfileInfo;
				if (bitfileInfo.Open(bitfilePath))
					cout	<< left << setw(int(maxlen)) << bitfilePath << "  "
							<< left << setw(11) << ::NTV2DeviceIDToString(bitfileInfo.GetDeviceID()) << " " << right
							<< bitfileInfo.GetDate() << " " << bitfileInfo.GetTime() << "  " << bitfileInfo.GetDesignName()
							<< endl;
				else
					cerr	<< "## ERROR:  Unable to open '" << bitfilePath << "' -- " << bitfileInfo.GetLastError() << endl;
			}
			else if (posMCS != string::npos  &&  (posMCS + 4) == bitfilePath.length())
			{
				CNTV2MCSfile	mcsInfo;
				if (mcsInfo.GetMCSHeaderInfo(bitfilePath))
					cout    << left << setw(int(maxlen)) << bitfilePath << "  "
							<< left << setw(11) << "---" << " " << right
							<< mcsInfo.GetBitfileDateString() << " " << mcsInfo.GetBitfileTimeString() << "  "
							<< left << setw(18) << mcsInfo.GetBitfileDesignString()
							<< "  " << mcsInfo.GetMCSPackageVersionString()
							<< "  " << mcsInfo.GetMCSPackageDateString() << endl;
				else
					cerr	<< "## ERROR:  Unable to open MCS bitfile '" << bitfilePath << "'" << endl;
			}
			else
				cerr	<< "## ERROR:  File '" << bitfilePath << "' doesn't end with '.bit' or '.mcs'" << endl;
		}
		if (!bQuiet && device.IsOpen() && ::NTV2DeviceHasSPIFlash(device.GetDeviceID()))
			ReportDeviceFlashStatus(device);

		if (device.IsIPDevice())
		{
			ULWord dnaLo;
			device.ReadRegister(kRegSarekDNALow + SAREK_REGS, dnaLo);
			ULWord dnaHi;
			device.ReadRegister(kRegSarekDNAHi + SAREK_REGS, dnaHi);
			cout << "Device DNA: " << HEX0N(dnaHi,8) << "-" << HEX0N(dnaLo,8) << endl;

			CNTV2KonaFlashProgram konaFlasher(device.GetIndexNumber());
			string licenseInfo;
			konaFlasher.ReadLicenseInfo(licenseInfo);
			cout << "License: " << licenseInfo << endl;

			ULWord licenseStatus;
			device.ReadRegister(kRegSarekLicenseStatus + SAREK_REGS, licenseStatus);
			cout << "Enable: 0x" << hex << (licenseStatus & 0xff)
				 << ((licenseStatus & SAREK_LICENSE_PRESENT) ? "" : " License not found")
				 << ((licenseStatus & SAREK_LICENSE_VALID) ? " License is valid" : " License NOT valid")
				 << endl;
		}
		return AJA_STATUS_SUCCESS;	//	Done!
	}
	else if (bitfilePaths.size() > 1)
	{
		cerr << "## ERROR:  More than one bitfile path specified" << endl;
		return AJA_STATUS_BAD_PARAM;
	}
	else if (!license.empty())
	{
		CNTV2KonaFlashProgram konaFlasher(device.GetIndexNumber());
		if (!konaFlasher.ProgramLicenseInfo(license))
			return AJA_STATUS_FAIL;
		cout << "Device license: OK" << endl;
		return AJA_STATUS_SUCCESS;
	}

	if (!device.IsOpen())
	{
		cerr << "## ERROR:  Device '" << deviceSpecifier << "' not found" << endl;
		return AJA_STATUS_OPEN;
	}
	if (bitfilePaths.empty())
	{
		cerr << "## ERROR:  No parameters - nothing to do." << endl;
		return AJA_STATUS_BAD_PARAM;	//	Done!
	}

	ostringstream	deviceInfo;
	deviceInfo << ::NTV2DeviceIDToString (device.GetDeviceID()) << " " << device.GetIndexNumber();

	if (!::NTV2DeviceHasSPIFlash (device.GetDeviceID()))
	{
		cerr << "## ERROR:  Device '" << deviceInfo.str() << "' is incapable of being flashed" << endl;
		return AJA_STATUS_UNSUPPORTED;
	}

	if (!bQuiet)
		ReportDeviceFlashStatus(device);

	//	Flash the device...
	CNTV2DeviceScanner				scanner;
	const NTV2DeviceInfo &			info			(scanner.GetDeviceInfoList()[device.GetIndexNumber()]);
	CNTV2FirmwareInstallerThread	installThread	(info, bitfilePaths.front(), bQuiet ? false : true, bForce ? true : false);

	AJAStatus result = installThread.Start();
	if (AJA_FAILURE (result))
	{
		cerr << "## ERROR:  Install thread failed to start" << endl;
		return AJA_STATUS_FAIL;
	}

	cout << "Installing firmware..." << endl;
	while (installThread.Active())
	{
		if (bProgress)
			{cout	<< ((installThread.GetProgressValue() * 100) / (installThread.GetProgressMax() ? installThread.GetProgressMax() : 1))
					<< "% " << installThread.GetStatusString() << "          \r";	cout.flush();}
		AJATime::Sleep (bProgress ? 1000 : 250);
	}

	if (bProgress)
		cout << installThread.GetStatusString() << "          " << endl;

	if (installThread.IsUpdateSuccessful())
	{
		if (!bQuiet)
		{
			ReportDeviceFlashStatus(device);
			cout << "## NOTE:  This host and/or AJA device must be power-cycled for the new firmware to load." << endl;
		}

		if (bWaitForEnterKey)
			CNTV2DemoCommon::WaitForEnterKeyPress();

		cout << "Firmware installed - OK" << endl;
		return AJA_STATUS_SUCCESS;
	}
	else
	{
		cerr << "## ERROR: Install thread failure." << endl;
		return AJA_STATUS_FAIL;
	}
}	//	main
