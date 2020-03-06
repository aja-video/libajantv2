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

using namespace std;

#ifdef MSWindows
#pragma warning(disable : 4996)
#endif

#ifdef AJALinux
#include "ntv2linuxpublicinterface.h"
#endif

static bool s_quit = false;

void SignalHandler(int signal)
{
	(void) signal;
	s_quit = true;
}

int main(int argc, const char ** argv)
{
	char* pDeviceSpec		(NULL);
	char* pDeviceID			(NULL);

	int deviceIndex			(0);
	NTV2DeviceID deviceID	((NTV2DeviceID)0);

	const struct poptOption userOptionsTable[] =
	{
        { "board", 'b', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pDeviceSpec, 'b', "device index", NULL },
        { "deviceID", 'd', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pDeviceID, 'd', "device ID", NULL },
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	poptContext		optionsContext;
	optionsContext = ::poptGetContext(NULL, argc, argv, userOptionsTable, 0);

	int rc;
	while ((rc = ::poptGetNextOpt(optionsContext)) >= 0)
	{
		switch (rc)
		{
		case 'b':
			if (pDeviceSpec == NULL)
			{
				fprintf(stderr, "## ERROR:  Must specify device index\n");
				return -1;
			}
			break;
		default:
			break;
		}
	}
	optionsContext = ::poptFreeContext(optionsContext);

	const std::string deviceSpec(pDeviceSpec ? pDeviceSpec : "");
	if (!deviceSpec.empty())
	{
		int tempData0 = 0;
		ULWord tempDataCount = 0;
		tempDataCount = sscanf(deviceSpec.c_str(), "%d", &tempData0);
		if (tempDataCount == 1)
		{
			deviceIndex = (ULWord)tempData0;
		}
		else
		{
			fprintf(stderr, "## ERROR:  Missing device index\n");
			return -1;
		}
	}

	const std::string deviceStr(pDeviceID ? pDeviceID : "");
    if (!deviceStr.empty())
	{
		ULWord tempData0 = 0;
		ULWord tempDataCount = 0;
        tempDataCount = sscanf(deviceStr.c_str(), "%x", &tempData0);
		if (tempDataCount == 1)
		{
			deviceID = (NTV2DeviceID)tempData0;
		}
	}

	try
	{
		signal(SIGINT, SignalHandler);

		NTV2DeviceInfo boardInfo;
		CNTV2DeviceScanner ntv2BoardScan;
		if(ntv2BoardScan.GetNumDevices() <= (ULWord)deviceIndex)
		{
			fprintf(stderr, "## ERROR:  Opening device %d failed\n", deviceIndex);
			throw 0;
		}
		boardInfo = ntv2BoardScan.GetDeviceInfoList()[deviceIndex];

		CNTV2Card ntv2dev(boardInfo.deviceIndex);

		// find the board
		if(ntv2dev.IsOpen() == false)
		{
			fprintf(stderr, "## ERROR:  Opening device %d failed\n", deviceIndex);
			throw 0;
		}
		NTV2DeviceID eBoardID = ntv2dev.GetDeviceID();

		if (s_quit) throw 0;
		
        printf("\nActive DeviceID: %08x   %s\n\n", eBoardID, NTV2DeviceIDToString(eBoardID).c_str());

		// scan the current directory for bitfiles.
        ntv2dev.AddDynamicDirectory(".");

        // check if requested device loadable
        if (deviceID != 0)
        {
            if (ntv2dev.CanLoadDynamicDevice(deviceID))
            {
                printf("Load DeviceID: %08x   %s\n", deviceID, NTV2DeviceIDToString(deviceID).c_str());
            }
            else
            {
                printf("Can not load DeviceID: %08x   %s\n", deviceID, NTV2DeviceIDToString(deviceID).c_str());
                deviceID = (NTV2DeviceID)0;
            }
        }

        // load requested device
        if (deviceID != 0)
        {
            if(ntv2dev.LoadDynamicDevice(deviceID))
            {
                eBoardID = ntv2dev.GetDeviceID();
                if (deviceID == eBoardID)
                {
                    printf("   Success - DeviceID: %08x   %s\n", eBoardID, NTV2DeviceIDToString(eBoardID).c_str());
                }
                else
                {
                    printf("   Unexpected device - DeviceID: %08x   %s\n", eBoardID, NTV2DeviceIDToString(eBoardID).c_str());
                }
            }
            else
            {
                eBoardID = ntv2dev.GetDeviceID();
                printf("   Load failed - DeviceID: %08x   %s\n", eBoardID, NTV2DeviceIDToString(eBoardID).c_str());
            }
        }

        // print loadable device list
		if (deviceID == 0)
		{
			std::vector<NTV2DeviceID> deviceList = ntv2dev.GetDynamicDeviceList();
			int dlSize = (int)deviceList.size();

			if (dlSize != 0)
			{
                printf("Can load DeviceID:\n");
                for (int i = 0; i < dlSize; i++)
				{
                    printf("   %08x   %s\n", (ULWord)deviceList[i], NTV2DeviceIDToString(deviceList[i]).c_str());
				}
                printf("\n");
            }
            else
            {
                printf("No loadable devices found\n");
            }

            return 0;
		}
		 
		return 0;
	}
	catch(...)
	{
	}

	return -1;
}
