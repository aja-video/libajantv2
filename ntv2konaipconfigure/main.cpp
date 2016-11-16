/**
	@file		ntv2konaipconfigure/main.cpp
	@copyright	Copyright (C) 2016 AJA Video Systems, Inc.  All rights reserved.
	@brief		Demonstrates how to programmatically configure the KonaIP's IP parameters. It sets up 2 receive channels
				(Input 1 and Input 2) and two transmit channels (Output 3 and Output 4).
	@note		Enable Transmit channel 4 only if you are sending something unique out Output 4.
**/

//	Includes
#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2config2022.h"
#include "ajabase/common/options_popt.h"

#define MICROBLAZE_UPTIME_REGISTER (0x40004)

using namespace std;


int main (int argc, const char ** argv)
{
	char *			pDeviceSpec		(NULL);		//	Which device to use
	poptContext		optionsContext; 			//	Context for parsing command line arguments

	//	Command line option descriptions:
	const struct poptOption userOptionsTable[] =
	{
		{ "board",	'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",	"index#, serial#, or model" },
		{ "device",	'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",	"index#, serial#, or model" },
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext(NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt(optionsContext);
	optionsContext = ::poptFreeContext(optionsContext);

	// Use rx_2022_channel to setup receive channels
	// NOTE: change values to match your IP infrastructure
	rx_2022_channel mChannel1Config;
	mChannel1Config.primaryDestIP = "239.16.2.210";
	mChannel1Config.primaryDestPort = 20210;
	mChannel1Config.primarySourceIP = "172.16.2.21";
	mChannel1Config.primarySourcePort = 10210;
	mChannel1Config.primaryRxMatch = RX_MATCH_DEST_IP; ///NOTE: you can add other criteria like RX_MATCH_DEST_PORT , see rx_2022_channel description for more

	rx_2022_channel mChannel2Config;
	mChannel2Config.primaryDestIP = "239.16.2.211";
	mChannel2Config.primaryDestPort = 20211;
	mChannel2Config.primarySourceIP = "172.16.2.21";
	mChannel2Config.primarySourcePort = 10211;
	mChannel2Config.primaryRxMatch = RX_MATCH_DEST_IP;

	// Use tx_2022_channel to setup transmit channels
	// NOTE: change values to match your IP infrastructure
	tx_2022_channel mChannel3Config;
	mChannel3Config.primaryRemoteIP = "239.16.2.230";
	mChannel3Config.primaryRemotePort = 20230;
	mChannel3Config.primaryLocalPort = 10230;
	mChannel3Config.primaryAutoMAC = true;

	tx_2022_channel mChannel4Config;
	mChannel4Config.primaryRemoteIP = "239.16.2.231";
	mChannel4Config.primaryRemotePort = 20231;
	mChannel4Config.primaryLocalPort = 10231;
	mChannel4Config.primaryAutoMAC = true;


	CNTV2Card mDevice;
	CNTV2DeviceScanner::GetFirstDeviceFromArgument (pDeviceSpec ? string (pDeviceSpec) : "0", mDevice);
	if (!mDevice.IsOpen())
		{cerr << "## ERROR:  No devices found" << endl;  return 2;}
	if ((mDevice.GetDeviceID() != DEVICE_ID_KONAIP_4CH_2SFP && mDevice.GetDeviceID() != DEVICE_ID_KONAIP_4CH_1SFP) || !mDevice.IsKonaIPDevice ())
		{cerr << "## ERROR:  Not a KONA IP device" << endl;  return 2;}

	//	Read MicroBlaze Uptime in Seconds, to see if it's running...
	while (!mDevice.IsDeviceReady ())
	{
		cerr << "## NOTE:  Waiting for device to become ready... (Ctrl-C will abort)" << endl;
		mDevice.SleepMs (1000);
		if (mDevice.IsDeviceReady ())
			cerr << "## NOTE:  Device is ready" << endl;
	}

	CNTV2Config2022	config2022 (mDevice);

	//	NOTE:	Make sure these are Unique! Change these addresses to match your IP infrastructure
	config2022.SetNetworkConfiguration (SFP_TOP,    "172.16.2.24", "255.255.0.0");
	config2022.SetNetworkConfiguration (SFP_BOTTOM, "172.16.2.23", "255.255.0.0");

	config2022.SetRxChannelConfiguration (NTV2_CHANNEL1, mChannel1Config);
	config2022.SetRxChannelEnable (NTV2_CHANNEL1, true,false);
	config2022.SetRxChannelConfiguration (NTV2_CHANNEL2, mChannel2Config);
	config2022.SetRxChannelEnable (NTV2_CHANNEL2, true,false);

	config2022.SetTxChannelConfiguration (NTV2_CHANNEL3, mChannel3Config);
	config2022.SetTxChannelEnable (NTV2_CHANNEL3, true,false);
	config2022.SetTxChannelConfiguration (NTV2_CHANNEL4, mChannel4Config);
	//	NOTE:	Enable a channel only if you are using it, otherwise you're sending unused bandwidth to the switch, etc.
	//config2022.SetTxChannelEnable (NTV2_CHANNEL4, true,false);
	config2022.SetTxChannelEnable (NTV2_CHANNEL4, false,false);

	return 0;

}	//	main
