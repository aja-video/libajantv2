/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2switchbitfile/main.cpp
	@brief		Demonstration application to change the active bitfile
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include <stdio.h>
#include <iostream>
#include <string>
#include <signal.h>

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2utils.h"
#include "ntv2bitfile.h"
#include "ntv2bitfilemanager.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/common/common.h"

using namespace std;

#ifdef MSWindows
	#pragma warning(disable : 4996)
#endif

#ifdef AJALinux
	#include "ntv2linuxpublicinterface.h"
#endif


int main(int argc, const char ** argv)
{
    char *			pDeviceSpec 	(AJA_NULL);			//	Device argument
    char *			pDeviceID	 	(AJA_NULL);			//	Device ID argument
    int				showVersion		(0);				//	Show version?
    int				isVerbose		(0);				//	Verbose output?
	int				isInfo			(0);				//	Info output?
	NTV2DeviceID	deviceID		(NTV2DeviceID(0));	//	Desired device ID to be loaded
	poptContext		optionsContext;						//	Context for parsing command line arguments
	int				resultCode		(0);

	const struct poptOption userOptionsTable[] =
	{
		{"version",	 0,  POPT_ARG_NONE   | POPT_ARGFLAG_OPTIONAL, &showVersion,	0,	"show version & exit",	AJA_NULL },
		{ "device",	'd', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pDeviceSpec,	0,	"device to use",		"index#, serial#, or model"	},
		{ "info",	'i', POPT_ARG_NONE   | POPT_ARGFLAG_OPTIONAL, &isInfo,		0,	"bitfile info?",		AJA_NULL },
		{ "load",	'l', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pDeviceID,	'l',"device ID to load",	"index# or hex32value" },
		{ "verbose",'v', POPT_ARG_NONE   | POPT_ARGFLAG_OPTIONAL, &isVerbose,	0,	"verbose output?",		AJA_NULL },
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);
	if (showVersion)
		{cout << argv[0] << ", NTV2 SDK " << ::NTV2Version() << endl;  return 0;}

	CNTV2Card device;
	const string deviceSpec(pDeviceSpec ? pDeviceSpec : "0");
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (deviceSpec, device))
		{cerr << "## ERROR: Opening device '" << deviceSpec << "' failed" << endl;  return 1;}
	NTV2DeviceID eBoardID (device.GetDeviceID());

	//	Scan the current directory for bitfiles...
	device.AddDynamicDirectory(".");

	const string deviceStr(pDeviceID ? pDeviceID : "");
    if (!deviceStr.empty())
	{
		size_t checkIndex(0);
		size_t index = aja::stoul(deviceStr, &checkIndex, 10);
		if (index < 100)
		{
			const NTV2DeviceIDList deviceList (device.GetDynamicDeviceList());
			if ((index == 0) || (index > deviceList.size()))
				{cerr << "## ERROR: Bad device index '" << index << "'" << endl;  return 1;}
			deviceID = deviceList.at(index-1);
		}
		else
		{
			deviceID = NTV2DeviceID(aja::stoul(deviceStr, &checkIndex, 16));
			isVerbose = true;
		}
	}

	if (isVerbose)
		cout << "Active device is " << ::NTV2DeviceIDToString(eBoardID) << " ("
				<< xHEX0N(eBoardID,8) << ")" << endl;
	else
		cout << "Active device is " << ::NTV2DeviceIDToString(eBoardID) << endl;

	do
	{
		//	Check if requested device loadable...
		if (deviceID)
		{
			if (!device.CanLoadDynamicDevice(deviceID))
			{
				cerr << "## ERROR: Cannot load device: " << ::NTV2DeviceIDToString(deviceID) << " ("
						<< xHEX0N(deviceID,8) << ")" << endl;
				deviceID = NTV2DeviceID(0);
			}
			else
			{
				if (isVerbose)
					cout << "Can load device: " << ::NTV2DeviceIDToString(deviceID) << " (" << xHEX0N(deviceID,8) << ")" << endl;
				else
					cout << "Can load device: " << ::NTV2DeviceIDToString(deviceID) << endl;
			}
		}

		//	Load requested device...
		if (deviceID)
		{
			if (!device.LoadDynamicDevice(deviceID))
			{
				eBoardID = device.GetDeviceID();
				cerr << "## ERROR: Load failed for device: " << ::NTV2DeviceIDToString(eBoardID)
						<< " (" << xHEX0N(eBoardID,8) << ")";
				resultCode = 2;
			}
			eBoardID = device.GetDeviceID();
			if (deviceID == eBoardID)
			{
				if (isVerbose)
					cout << "Device: " << ::NTV2DeviceIDToString(eBoardID) << " (" << xHEX0N(eBoardID,8)
							<< ") loaded successfully" << endl;
				else
					cout << "Device: " << ::NTV2DeviceIDToString(eBoardID) << " loaded successfully" << endl;
			}
			else
			{
				cerr << "## ERROR: Unexpected device: " << ::NTV2DeviceIDToString(eBoardID)
						<< " (" << xHEX0N(eBoardID,8) << ")";
				resultCode = 3;
			}
		}

		//	Print loadable device list...
		if (!deviceID && !isInfo)
		{
			const NTV2DeviceIDList deviceList (device.GetDynamicDeviceList());
			if (deviceList.empty())
				{cout << "No loadable devices found" << endl;  break;}

			cout << DEC(deviceList.size()) << " Device(s) for dynamic loading:" << endl;
			for (size_t ndx(0);  ndx < deviceList.size();  ndx++)
			{
				if (isVerbose)
					cout << DECN(ndx+1,2) << ": " << ::NTV2DeviceIDToString(deviceList.at(ndx))
						<< " (" << xHEX0N(deviceList.at(ndx),8) << ")" << endl;
				else
					cout << DECN(ndx+1,2) << ": " << ::NTV2DeviceIDToString(deviceList.at(ndx)) << endl;
			}
		}

		// Print detailed bitfile info
		if (isInfo)
		{
			//	Get current design ID and version...
			NTV2ULWordVector reg;
			device.BitstreamStatus(reg);
			ULWord designID (NTV2BitfileHeaderParser::GetDesignID(reg[BITSTREAM_VERSION]));
			ULWord designVersion (NTV2BitfileHeaderParser::GetDesignVersion(reg[BITSTREAM_VERSION]));
			NTV2DeviceID currentDeviceID (device.GetDeviceID());
			ULWord bitfileID (CNTV2Bitfile::ConvertToBitfileID(currentDeviceID));
			UWord bitfileVersion(0);
			string flags = " Active";
			device.GetRunningFirmwareRevision(bitfileVersion);
			cout << std::setw(20) << std::left << ::NTV2DeviceIDToString(currentDeviceID) << std::right
				 << " DevID " << xHEX0N(currentDeviceID,8)
				 << "  DesID " << xHEX0N(designID,2) << "  DesVer " << xHEX0N(designVersion,2)
				 << "  BitID " << xHEX0N(bitfileID,2) << "  BitVer " << xHEX0N(bitfileVersion,2)
				 << " " << flags << endl;

			CNTV2BitfileManager bitMan;
			bitMan.AddDirectory(".");
			const NTV2BitfileInfoList infoList(bitMan.GetBitfileInfoList());
			for (NTV2BitfileInfoListConstIter it(infoList.begin());  it != infoList.end();  ++it)
			{
				flags = "";
				if (it->bitfileFlags & NTV2_BITFILE_FLAG_TANDEM)
					flags += " Tandem";
				if (it->bitfileFlags & NTV2_BITFILE_FLAG_PARTIAL)
					flags += " Partial";
				if (it->bitfileFlags & NTV2_BITFILE_FLAG_CLEAR)
					flags += " Clear";
				cout << std::setw(20) << std::left << ::NTV2DeviceIDToString(it->deviceID) << std::right
					 << " DevID " << xHEX0N(it->deviceID,8)
					 << "  DesID " << xHEX0N(it->designID,2) << "  DesVer " << xHEX0N(it->designVersion,2)
					 << "  BitID " << xHEX0N(it->bitfileID,2) << "  BitVer " << xHEX0N(it->bitfileVersion,2)
					 << " " << flags << endl;
			}
		}
		
	} while (false);

	return resultCode;
}	//	main
