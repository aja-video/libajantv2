/**
	@file		ntv2testOGL/main.cpp
	@copyright	(C) 2012-2020 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "../democlasses/ntv2capture.h"
#include <signal.h>
#include <iostream>
#include <iomanip>

//#include "gpustuff/utility/oglPipeline.h"
#include "gpustuff/include/gpuTransferInterface.h"
#include "gpustuff/include/cpuObject.h"
#include "gpustuff/include/oglObject.h"
#include "gpustuff/include/oglTransfer.h"


//GpuTransfer<COglObject>		oglXfer;
IGpuTransfer<COglObject>		*pOglXfer;


//	Globals
static bool	gGlobalQuit		(false);	//	Set this "true" to exit gracefully


void SignalHandler (int inSignal)
{
	gGlobalQuit = true;
}


int main (int argc, const char ** argv)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);
	uint32_t		boardNumber		(0);					//	Which board to use
	poptContext		optionsContext;							//	Context for parsing command line arguments

	//
	//	Command line option descriptions:
	//
	const struct poptOption userOptionsTable [] =
	{
		{"board",	'b',	POPT_ARG_INT,	&boardNumber,	0,	"which board to use",	"number of the board"  },		//	--board N
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//
	//	Read command line arguments...
	//
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);


	pOglXfer = CreateOglTransfer();
	pOglXfer->Init();
	

//	m_GpuTransfer = CreateOglTransfer();
//	m_GpuTransfer->Init();



	//
	//	Instantiate the NTV2Capture object, using the specified AJA device...
	//
	NTV2Capture	capturer (boardNumber);

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the NTV2Capture instance...
	status = capturer.Init ();
	if (AJA_SUCCESS (status))
	{
		//	Run the burner...
		capturer.Run ();

		//	Poll its status until stopped...
		AUTOCIRCULATE_STATUS_STRUCT	inputStatus;
		bool						firstTimeAround	(true);

		//	Loop until someone tells us to stop...
		while (gGlobalQuit == false)
		{
			capturer.GetACStatus (inputStatus);
			if (firstTimeAround)
			{
				cout	<< "           Capture  Capture" << endl
						<< "   Frames   Frames   Buffer" << endl
						<< "Processed  Dropped    Level" << endl;
				firstTimeAround = false;
			}

			cout	<< setw (9) << inputStatus.framesProcessed
					<< setw (9) << inputStatus.framesDropped
					<< setw (9) << inputStatus.bufferLevel
					<< "\r" << flush;

			AJATime::Sleep (500);

		}	//	loop until signaled

		cout << endl;

	}	//	if Init succeeded
	else
		cout << "Capture initialization failed with status " << status << endl;

	return 1;

}	//	main
