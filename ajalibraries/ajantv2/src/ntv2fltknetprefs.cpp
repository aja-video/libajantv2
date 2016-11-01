/**
	@file		ntv2fltknetprefs.cpp
	@brief		Implements the CNTV2FLTKNetPrefs class.
	@copyright	(C) 2008-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include <string.h> // memset(), memcmp(), memcpy()
#include <stdlib.h> // free()

#include "ntv2fltknetprefs.h"


// Initialization of static members

const char *CNTV2FLTKNetPrefs::chosenBoardNumberKey = "chosenBoardNumber";
const char *CNTV2FLTKNetPrefs::chosenBoardTypeKey = "chosenBoardType";
const char *CNTV2FLTKNetPrefs::chosenBoardIDKey = "chosenBoardBoardID";
const char *CNTV2FLTKNetPrefs::chosenBoardDescriptionKey = "chosenBoardDescription";
const char *CNTV2FLTKNetPrefs::chosenBoardHostnameKey = "chosenBoardHostname";
const char *CNTV2FLTKNetPrefs::numHostnamesKey = "numHostnames";
const char *CNTV2FLTKNetPrefs::hostnameItemKey = "hostnameItem";
const char *CNTV2FLTKNetPrefs::networkTimeoutKey = "networkTimeout";

void
CNTV2FLTKNetPrefs::save(Fl_Preferences &pref, const char *configName,
				int networkTimeout,
				LWord chosenBoard,
				NTV2ChoosableBoard availableBoards[MAX_CHOOSABLE_BOARDS],
				Fl_Choice *hostNameChoice)
{
	// Empty out previous configuration
	//removeConfiguration(pref, configName ); Causes a segfault in the next line under Fltk 1.3.0

	Fl_Preferences configGroup(pref, configName);

    configGroup.set(networkTimeoutKey, networkTimeout);
	
	if (chosenBoard != NO_BOARD_CHOSEN)
	{
		configGroup.set( chosenBoardNumberKey, (int)availableBoards[chosenBoard].boardNumber);
		configGroup.set( chosenBoardTypeKey, (int)availableBoards[chosenBoard].boardType);
		configGroup.set( chosenBoardDescriptionKey, availableBoards[chosenBoard].description);
		configGroup.set( chosenBoardIDKey, availableBoards[chosenBoard].boardID);
		configGroup.set( chosenBoardHostnameKey, availableBoards[chosenBoard].hostname);
	}

	if (hostNameChoice)
	{
		int howMany = hostNameChoice->size(); 
		for (int i = 0; i < howMany; i++)
		{
			const char *hostname = hostNameChoice->text(i);
    		if (hostname && strcmp(hostname,"255.255.255.255") && strcmp(hostname,"localhost"))
    		{
				if (i == 2) // Skip broadcast and localhost
				{
					configGroup.set( numHostnamesKey, howMany - 3);
				}
				configGroup.set(Fl_Preferences::Name("%s%d", hostnameItemKey, i-2),hostname);
    		}
		}
	}
}

void 
CNTV2FLTKNetPrefs::removeConfiguration(Fl_Preferences &configFile, const char *configName )
{
	// this looks weird but works
	Fl_Preferences configGroup( configFile, configName );
	configFile.deleteGroup( configName );
}

void
CNTV2FLTKNetPrefs::restore(Fl_Preferences &pref, const char *configName,
					Fl_Value_Input *networkTimeout,
					NTV2ChoosableBoard availableBoards[MAX_CHOOSABLE_BOARDS],
					ULWord &numAvailableBoards,
    				Fl_Choice *hostAndBoardChoice,
					LWord &chosenBoard,
					Fl_Choice *hostNameChoice,
					bool &cardChangeRequired 
					)
{
	Fl_Preferences configGroup( pref, configName );
	int found, userVal;

	cardChangeRequired = false;
    found = configGroup.get(networkTimeoutKey, userVal, networkTimeout ? (int) networkTimeout->value() : 0);
	if (found && networkTimeout)
	{
    	networkTimeout->value(userVal);
	}

	// If we were connected to a board before, attempt to reconnect.
    found = configGroup.get(chosenBoardNumberKey, userVal, 0);
	if (found)
	{
		NTV2ChoosableBoard savedBoard;
		memset(&savedBoard, 0, sizeof(NTV2ChoosableBoard));

		// Get saved board info
		if( configGroup.get( chosenBoardNumberKey, (int&)savedBoard.boardNumber, 0) &&
			configGroup.get( chosenBoardTypeKey, (int&)savedBoard.boardType, 0) &&
			configGroup.get( chosenBoardDescriptionKey, savedBoard.description, 0, NTV2_DISCOVER_BOARDINFO_DESC_STRMAX - 1) &&
			configGroup.get( chosenBoardIDKey, (int&)savedBoard.boardID, 0) &&
			configGroup.get( chosenBoardHostnameKey, savedBoard.hostname, 0, CHOOSABLE_BOARD_STRMAX - 1))
		{
			// See if it is already there
			bool foundIt = false;
			for (int i = 0; i < numAvailableBoards; i++)
			{
				if (memcmp(&availableBoards[i], &savedBoard, sizeof(NTV2ChoosableBoard)) == 0)
				{
					// Already there, use it
					hostAndBoardChoice->value(i);
					chosenBoard = savedBoard.boardNumber;
					cardChangeRequired=true;
					foundIt = true;
				}
			}
			// Not there, add it if there is space
			if (!foundIt && numAvailableBoards < MAX_CHOOSABLE_BOARDS)
			{
				if (numAvailableBoards == 0)
				{
    				hostAndBoardChoice->clear(); // Get rid of "No boards found" entry.
				}

				memcpy(&availableBoards[numAvailableBoards], &savedBoard, sizeof(NTV2ChoosableBoard));
           		hostAndBoardChoice->add(availableBoards[numAvailableBoards].description);
				++numAvailableBoards;
				chosenBoard = hostAndBoardChoice->size() - 2; 	// size() returns 0 if null, 1 if empty.
				hostAndBoardChoice->value(chosenBoard);
				cardChangeRequired=true;
			}
		}
	}

	if (hostNameChoice)
	{
		int howMany;
		found = configGroup.get(numHostnamesKey, howMany, 0);
		if (found)
		{
			for (int i = 0; i < howMany; i++)
			{
				char *hostname = 0;
				found = configGroup.get(Fl_Preferences::Name("%s%d", hostnameItemKey, i), hostname, 0);
				if (found && hostname)
				{
					// configGroup.set(Fl_Preferences::Name("%s%d", hostnameItemKey, i-2),hostname);
					hostNameChoice->add(hostname);
					free(hostname);
				}
    		}
		}
	}
}

