// 
// Copyright (C) 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.
//

// Very small test program for new NTV2 GU stuff.
// This will go away when the ntv2*.cpp files are
// compiled into a library -- STC

#include <iostream>

#include "ntv2card.h"

int main(int argc, char *argv[])
{
	CNTV2Card *card = (CNTV2Card *)new CNTV2Card();
 
	if (card == NULL) 
	{
	   cout << "Unable to create a CNTV2Card object!" << endl;
	   return 1;
	}

	cout << "Successfully created a CTNV2Card object." << endl;
	bool result = card->Open(0, true);
	cout << "Result of open is " << (result ? "true" : "false") << endl;

	string str;
	card->GetDriverVersionString(str);
	cout << "Driver version is " << str.c_str() << endl;
	
	cout << "Initializing AutoCirculate" << endl;
	result = card->InitAutoCirculate( NTV2CROSSPOINT_CHANNEL1,
                                    0, 
                                    1,
                                    false,
                                    false);
	cout << "Result of InitAutoCirculate() is " << (result ? "true" : "false") << endl;

	cout << "Getting AutoCirculate status" << endl;
	AUTOCIRCULATE_STATUS_STRUCT autoCirculateStatus;

	result = card->GetAutoCirculate(NTV2CROSSPOINT_CHANNEL1, &autoCirculateStatus);
	cout << "Result of GetAutoCirculate() is " << (result ? "true" : "false") << endl;
	
	cout << "Getting frame stamp for channel one, frame zero" << endl;
	FRAME_STAMP_STRUCT pFrameStamp;
	result = card->GetFrameStamp(NTV2CROSSPOINT_CHANNEL1, 0, &pFrameStamp);
	cout << "Result of GetFrameStamp() is " << (result ? "true" : "false") << endl;

	result = card->Close();
	cout << "Result of close is " << (result ? "true" : "false") << endl;
	delete card;
	cout << "Deleted it." << endl;
	cout << "The end." << endl;
	return 0;
}
