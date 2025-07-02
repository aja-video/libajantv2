/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2metale2e/main.cpp
    @brief		Simple Xilinx Metal demonstration for E2E setup on power.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2metale2e.h"


using namespace std;


int main (int argc, const char ** argv)
{	//	Create the object that will display the test pattern...
    NTV2MetalE2E powerUpDemo(config);
    AJAStatus status = powerUpDemo.Init();

    //	Setup the genlock, route the SDI In 1 to SDI out 2, setup the output
    status = powerUpDemo.doSomething();
	if (AJA_FAILURE(status))
        {cout << "## ERROR:  doSomething failed: " << ::AJAStatusToString(status) << endl;	return 2;}

	return 0;

}	// main
