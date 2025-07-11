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
    NTV2MetalE2E powerUpDemo;
    AJAStatus status = powerUpDemo.DoSomething();

	if (AJA_FAILURE(status))
        {cout << "## ERROR:  doSomething failed: " << ::AJAStatusToString(status) << endl;	return 2;}

	return 0;

}	// main
