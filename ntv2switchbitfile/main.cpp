/**
	@file		ntv2switchbitfile/main.cpp
	@brief		Demonstration application to change the active bitfile
	@copyright	(C) 2012-2020 AJA Video Systems, Inc.  All rights reserved.
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
    char *			pDeviceSpec 	(AJA_NULL);				//	Device argument
    char *			pDeviceID	 	(AJA_NULL);				//	Device ID argument
    int				isVerbose		(0);					//	Verbose output?
	NTV2DeviceID	deviceID		(NTV2DeviceID(0));		//	Desired device ID to be loaded
	poptContext		optionsContext;							//	Context for parsing command line arguments
	int				resultCode		(0);

	const struct poptOption userOptionsTable[] =
	{
		{ "device",	'd', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pDeviceSpec,	0,	"which device to use",	"index#, serial#, or model"	},
		{ "load",	'l', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pDeviceID,	'l',"device ID to load",	"hex32value" },
		{ "verbose",'v', POPT_ARG_NONE   | POPT_ARGFLAG_OPTIONAL, &isVerbose,	0,	"verbose output?",		AJA_NULL },
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	CNTV2Card device;
	const string deviceSpec(pDeviceSpec ? pDeviceSpec : "0");
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (deviceSpec, device))
		{cerr << "## ERROR: Opening device '" << deviceSpec << "' failed" << endl;  return 1;}
	NTV2DeviceID eBoardID (device.GetDeviceID());

	const string deviceStr(pDeviceID ? pDeviceID : "");
    if (!deviceStr.empty())
	{
		size_t checkIndex(0);
		deviceID = NTV2DeviceID(aja::stoul(deviceStr, &checkIndex, 16));
	}

	if (isVerbose)
		cout << "## NOTE: Active device is " << xHEX0N(eBoardID,8) << " ("
				<< ::NTV2DeviceIDToString(eBoardID) << ")" << endl;
	do
	{
		//	Scan the current directory for bitfiles...
        device.AddDynamicDirectory(".");

		//	Check if requested device loadable...
		if (deviceID)
		{
			if (!device.CanLoadDynamicDevice(deviceID))
			{	cerr << "Cannot load DeviceID: " << xHEX0N(deviceID,8) << " ("
						<< ::NTV2DeviceIDToString(deviceID) << ")" << endl;
				deviceID = NTV2DeviceID(0);
			}
			else cout << "Can load DeviceID: " << xHEX0N(deviceID,8) << " (" << ::NTV2DeviceIDToString(deviceID) << ")" << endl;
		}

		//	Load requested device...
		if (deviceID)
		{
			if (!device.LoadDynamicDevice(deviceID))
			{
				eBoardID = device.GetDeviceID();
				cerr << "## ERROR:  Load failed for DeviceID: " << xHEX0N(eBoardID,8)
						<< " (" << ::NTV2DeviceIDToString(eBoardID) << ")";
				resultCode = 2;
			}
			eBoardID = device.GetDeviceID();
			if (deviceID == eBoardID)
				cout << "DeviceID: " << xHEX0N(eBoardID,8) << " (" << ::NTV2DeviceIDToString(eBoardID)
						<< ") loaded successfully" << endl;
			else
			{
				cerr << "## ERROR: Unexpected DeviceID: " << xHEX0N(eBoardID,8)
						<< " (" << ::NTV2DeviceIDToString(eBoardID) << ")";
				resultCode = 3;
			}
		}

		//	Print loadable device list...
		if (!deviceID)
		{
			const NTV2DeviceIDList deviceList (device.GetDynamicDeviceList());
			if (deviceList.empty())
				{cout << "## NOTE:  No loadable devices found" << endl;  break;}

			cout << DEC(deviceList.size()) << " DeviceID(s) for dynamic loading:" << endl;
			for (size_t ndx(0);  ndx < deviceList.size();  ndx++)
				cout << DECN(ndx+1,2) << ": " << xHEX0N(deviceList.at(ndx),8)
					<< " (" << ::NTV2DeviceIDToString(deviceList.at(ndx)) << endl;
		}
		
	} while (false);

	return resultCode;
}	//	main
