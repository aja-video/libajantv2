/**
	@file		qamailbox/main.cpp
	@brief		Neither snow nor rain nor heat nor gloom of night will keep this from testing your mail box.
	@copyright	Copyright (C) 2012-2016 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include "ajatypes.h"
#include "ntv2devicescanner.h"
#include "ntv2enhancedcsc.h"
#include "ntv2utils.h"
#include "ajabase/common/options_popt.h"
#include <signal.h>
#include <iostream>
#include <iomanip>
#include "ajabase/system/systemtime.h"

using namespace std;

//	Globals

int main (int argc, const char ** argv)
{
	CNTV2Card		mDevice;
	char *			pDeviceSpec(NULL);		//	Which device to use
	poptContext		optionsContext; 		//	Context for parsing command line arguments
	bool			bVerbose(false);
	bool			bQuiet(false);

	ULWord			oldColors [4];
	ULWord			newColors [4];

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{ "device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use"	},
		{ "verbose",	'v',	POPT_ARG_NONE,		&bVerbose,		0,	"Verbose output"		},
		{ "quiet",		'a',	POPT_ARG_NONE,		&bQuiet,		0,	"Quiet output"			},

		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	std::string mDeviceSpecifier = pDeviceSpec ? pDeviceSpec : "0";

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, mDevice))
	{
		cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;
	}

	//	Read the matte color registers the old way...
	mDevice.ReadRegister (kRegFlatMatteValue,  &oldColors [0]);
	mDevice.ReadRegister (kRegFlatMatte2Value, &oldColors [1]);
	mDevice.ReadRegister (kRegFlatMatte3Value, &oldColors [2]);
	mDevice.ReadRegister (kRegFlatMatte4Value, &oldColors [3]);
	cout << "Original colors: " << hex << oldColors[0] << ", " << oldColors[1] << ", " << oldColors[2] << ", " << oldColors[3] << endl;

	//	Read the matte color registers the new way...
	//						(RegisterNumber,	RegValue)
 	NTV2RegInfo	bankSel	(kRegFlatMatteValue,	0xaaaaaaaa);
	NTV2RegInfo	regInfo	(kRegFlatMatte2Value,	0x137);
	bool rv = mDevice.BankSelectReadRegister (bankSel, regInfo);
	if (rv)
	    newColors [0]  = regInfo.registerValue;
	else
		cerr << "Error reading newColor[0]" << endl;

	bankSel.Set (kRegFlatMatteValue,	0xbbbbbbbb);
	regInfo.Set (kRegFlatMatte3Value,	0x137);
	rv = mDevice.BankSelectReadRegister (bankSel, regInfo);
	if (rv)
	    newColors [1]  = regInfo.registerValue;
	else
		cerr << "Error reading newColor[1]" << endl;

	bankSel.Set (kRegFlatMatteValue, 0xcccccccc);
	regInfo.Set (kRegFlatMatte4Value, 0x137);
	rv = mDevice.BankSelectReadRegister (bankSel, regInfo);
	if (rv)
	    newColors [2]  = regInfo.registerValue;
	else
		cerr << "Error reading newColor[2]" << endl;

	cout << "Original new colors: " << hex << newColors[0] << ", " << newColors[1] << ", " << newColors[2] << ", " << newColors[3] << endl;

	//	Write the matte color registers the new way...
	bankSel.Set (kRegFlatMatteValue, 0xdddddddd);
	regInfo.Set (kRegFlatMatte2Value, 0x11111111);
	rv = mDevice.BankSelectWriteRegister (bankSel, regInfo);
	if (!rv)
		cerr << "Error writing newColor[0]" << endl;

	bankSel.Set (kRegFlatMatteValue, 0xeeeeeeee);
	regInfo.Set (kRegFlatMatte3Value, 0x22222222);
	rv = mDevice.BankSelectWriteRegister (bankSel, regInfo);
	if (!rv)
		cerr << "Error writing newColor[1]" << endl;

	bankSel.Set (kRegFlatMatteValue, 0xffffffff);
	regInfo.Set (kRegFlatMatte4Value, 0x33333333);
	rv = mDevice.BankSelectWriteRegister (bankSel, regInfo);
	if (!rv)
		cerr << "Error writing newColor[2]" << endl;

	//	Read the matte color registers the old way...
	mDevice.ReadRegister (kRegFlatMatteValue,  &newColors [0]);
	mDevice.ReadRegister (kRegFlatMatte2Value, &newColors [1]);
	mDevice.ReadRegister (kRegFlatMatte3Value, &newColors [2]);
	mDevice.ReadRegister (kRegFlatMatte4Value, &newColors [3]);
	cout << "Readback colors: " << hex << newColors[0] << ", " << newColors[1] << ", " << newColors[2] << ", " << newColors[3] << endl;

	//	Try to get the mail box lock
	rv = mDevice.AcquireMailBoxLock();
	if (rv)
		cout << "Got the lock, rv " << rv << endl;
	else
		cerr << "## ERROR:  Could not get the lock, rv " << rv << endl;

	rv = mDevice.AcquireMailBoxLock();
	if (rv)
		cerr << "## ERROR:  Got the lock twice (a bad thing), rv " << rv << endl;
	else
		cout << "Could not get the lock twice (a good thing), rv " << rv << endl;;

	ULWord oldTimeoutVal;
	mDevice.ReadRegister (kRegMailBoxTimeoutNS, &oldTimeoutVal);
	cout << "Current timeout " << oldTimeoutVal << " ns" << endl;

	//	set 3 second timeout in units of 100 ns
	ULWord newTimeoutVal;
	mDevice.WriteRegister (kRegMailBoxTimeoutNS, (ULWord)3 * 1000 * 1000 * 10);
	mDevice.ReadRegister (kRegMailBoxTimeoutNS, &newTimeoutVal);
	cout << "New timeout " << newTimeoutVal << " ns" << endl;

	rv = mDevice.AcquireMailBoxLock();
	if (rv)
		cerr << "## ERROR:  Got the lock after new timeout (bad), rv " << rv << endl;
	else
		cout << "Could not get the lock after new timeout (good), rv " << rv << endl;

	mDevice.WriteRegister (kRegMailBoxTimeoutNS, oldTimeoutVal);

	mDevice.ReleaseMailBoxLock();

	rv = mDevice.AcquireMailBoxLock();
	if (rv)
		cout << "Got the lock after release (good), rv " << rv << endl;
	else
		cerr << "## ERROR:  Could not get the lock after release (bad), rv " << rv << endl;

	mDevice.ReleaseMailBoxLock();

	//	Restore the matte color registers the old way
	mDevice.WriteRegister (kRegFlatMatteValue,  oldColors [0]);
	mDevice.WriteRegister (kRegFlatMatte2Value, oldColors [1]);
	mDevice.WriteRegister (kRegFlatMatte3Value, oldColors [2]);
	mDevice.WriteRegister (kRegFlatMatte4Value, oldColors [3]);

	//	Read the matte color registers the old way
	mDevice.ReadRegister (kRegFlatMatteValue,  &oldColors [0]);
	mDevice.ReadRegister (kRegFlatMatte2Value, &oldColors [1]);
	mDevice.ReadRegister (kRegFlatMatte3Value, &oldColors [2]);
	mDevice.ReadRegister (kRegFlatMatte4Value, &oldColors [3]);
	cout << "Final colors: " << hex << oldColors[0] << ", " << oldColors[1] << ", " << oldColors[2] << ", " << oldColors[3] << endl;
 
	return 0;
}	//	main

