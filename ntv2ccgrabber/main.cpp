/**
	@file		ntv2ccgrabber/main.cpp
	@brief		Demonstration application that grabs closed-captioning data from frames captured
				from SDI input (using AutoCirculate), and writes the captions to standard output.
	@copyright	Copyright (C) 2013-2017 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/pnp/pnp.h"
#include "ntv2ccgrabber.h"
#include "ntv2utils.h"
#include <signal.h>
#include <iostream>
#include <iomanip>
#if defined (MSWindows) || defined (AJAWindows)
	#define	SIGQUIT	SIGBREAK
#endif


//	Globals
static bool		gGlobalQuit		(false);	//	Set this "true" to exit gracefully
static AJAPnp	gPlugAndPlay;				//	To detect device disconnects


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
	gPlugAndPlay.Uninstall ();
}


/**
	@brief	This function gets called whenever an AJA device is attached or detached to/from the host.
	@param[in]	inMessage	Specifies if a device was attached or detached.
	@param[in]	pUserData	The client data associated with the callback.
**/
static void PnpCallback (const AJAPnpMessage inMessage, void * pUserData)		//	static
{
	(void) pUserData;
	cerr << "## PnpCallback:  " << inMessage << endl;
	if (inMessage == AJA_Pnp_DeviceAdded || inMessage == AJA_Pnp_DeviceRemoved)
	{
		static unsigned	sAttaches	(0);
		static unsigned	sDetaches	(0);
		if (inMessage == AJA_Pnp_DeviceAdded)
			sAttaches++;
		if (inMessage == AJA_Pnp_DeviceRemoved)
			sDetaches++;
		if (sAttaches > 1 || inMessage == AJA_Pnp_DeviceRemoved)
		{
			SignalHandler (SIGQUIT);
			cerr << "## WARNING:  Terminating 'ntv2ccgrabber' due to device " << (inMessage == AJA_Pnp_DeviceAdded ? "attach" : "detach") << endl;
		}
	}

}	//	PnpCallback



int main (int argc, const char ** argv)
{
	const string	legalChannels	(NTV2CCGrabber::GetLine21ChannelNames ("|"));
	AJAStatus		status			(AJA_STATUS_SUCCESS);	//	Init result
	char *			pDeviceSpec		(NULL);					//	Which device to use
	uint32_t		inputNumber		(1);					//	Which SDI input to capture from (1 thru 8)
	int				burnCaptions	(0);					//	Burn-in captions?
	int				bMultiFormat	(0);					//	Enable multi-format?
	int				bForceVanc		(0);					//	Force use of Vanc?
	int				bWithAudio		(0);					//	Grab audio?
	char *			pCaptionChannel	(NULL);					//	Caption channel of interest (cc1, cc2 ... text1, text2, ...)
	poptContext		optionsContext;							//	Context for parsing command line arguments

	::setlocale (LC_ALL, "");	//	Might have to emit UTF-8 Unicode

	gPlugAndPlay.Install (PnpCallback, 0, AJA_Pnp_PciVideoDevices);

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device to use",			"index#, serial#, or model"	},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device to use",			"index#, serial#, or model"	},
		{"608chan",		0,		POPT_ARG_STRING,	&pCaptionChannel,	0,	"608 caption channel",			legalChannels.c_str ()},
		{"burn",		0,		POPT_ARG_NONE,		&burnCaptions,		0,	"burn-in captions",				NULL						},
		{"input",		'i',	POPT_ARG_INT,		&inputNumber,		0,	"which SDI input",				"1 thru 8"					},
		{"multiChannel",'m',	POPT_ARG_NONE,		&bMultiFormat,		0,	"enables multi-channel/format",	NULL},
		{"vanc",		'v',	POPT_ARG_NONE,		&bForceVanc,		0,	"force use of vanc",			NULL},
		{"audio",		'a',	POPT_ARG_NONE,		&bWithAudio,		0,	"grab audio",					NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	NTV2Line21Channel	captionChannel	(pCaptionChannel ? ::StrToNTV2Line21Channel (string (pCaptionChannel)) : NTV2_CC608_CC1);
	if (captionChannel == NTV2_CC608_ChannelMax)
		{cerr << "## ERROR:  Bad '608chan' value '" << pCaptionChannel << "' -- expected '" << legalChannels << "'" << endl;	return 1;}
	if (inputNumber == 0 || inputNumber > 8)
		{cerr << "## ERROR:  Bad 'input' value '" << inputNumber << "' -- expected 1 thru 8" << endl;	return 1;}

	const NTV2Channel		inputChannel	(static_cast <NTV2Channel> (inputNumber - 1));

	//	Instantiate the NTV2CCGrabber, using the specified AJA device and input source...
	NTV2CCGrabber	ccGrabber (pDeviceSpec ? string (pDeviceSpec) : "0",	//	which device?
								captionChannel,								//	which caption channel to listen to?
								burnCaptions ? true : false,				//	burn in captions?
								bMultiFormat ? true : false,				//	multiformat/multichannel?
								bForceVanc ? true : false,					//	force use of Vanc instead of using Anc extractor?
								bWithAudio ? true : false,					//	grab audio?
								inputChannel);								//	which input channel (and SDI input) to use?

	//	Initialize the ccGrabber instance...
	status = ccGrabber.Init ();
	if (AJA_FAILURE (status))
		{cerr << "## ERROR:  'ntv2ccgrabber' initialization failed with status " << status << endl;	return 1;}

	//	Run the ccGrabber...
	ccGrabber.Run ();

	//	Loop until someone tells us to stop...
	while (!gGlobalQuit)
	{
		const char	keyPressed	(CNTV2DemoCommon::ReadCharacterPress ());
		if (keyPressed == 'q' || keyPressed == 'Q')
			SignalHandler (SIGQUIT);
		else if (keyPressed >= '1' && keyPressed <= '9')
			ccGrabber.SetCaptionDisplayChannel (NTV2Line21Channel (keyPressed - '1'));
		else if (keyPressed == '?')
			cerr << endl << "## HELP:  1=CC1 2=CC2 3=CC3 4=CC4 5=Txt1 6=Txt2 7=Txt3 8=Txt4 9=XDS Q=Quit H=HUD ?=Help" << endl;
		else if (keyPressed == 'h' || keyPressed == 'H')
			ccGrabber.ToggleHUD ();
		else if (keyPressed == 'v' || keyPressed == 'V')
			ccGrabber.ToggleVANC ();
		AJATime::Sleep (500);
	}
	cerr << endl;
	return 0;

}	//	main
