/**
	@file		ntv2fltknetprefs.h
	@brief		Declares the CNTV2FLTKNetPrefs class.
	@copyright	(C) 2008-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2FLTKNETPREFS_H
#define NTV2FLTKNETPREFS_H

/* FLTK headers */

#include <FL/Fl.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Value_Input.H>

#include "ajatypes.h"
//#include "ntv2enums.h"
#include "ntv2nubtypes.h"
#include "ntv2choosableboard.h"

// Preference ID space

#define COMPANY_ID	"aja"
#define CONFIG_NAME_HOST_BOARD_NETWORK "HostBoardNetwork"

/**
	@brief	This class is used to save and restore network "nub" preferences for the FLTK-based Watcher and Cables apps.
**/
class CNTV2FLTKNetPrefs
{
	public:

	CNTV2FLTKNetPrefs() {};
	virtual ~CNTV2FLTKNetPrefs() {};

	void save(Fl_Preferences &pref, const char *configName,
				int networkTimeout,
				LWord chosenBoard,
				NTV2ChoosableBoard availableBoards[MAX_CHOOSABLE_BOARDS],
				Fl_Choice *hostNameChoice);

	void restore(Fl_Preferences &configFile, const char *configName,
					Fl_Value_Input *networkTimeout,
					NTV2ChoosableBoard availableBoards[MAX_CHOOSABLE_BOARDS],
					ULWord &numAvailableBoards,
    				Fl_Choice *hostAndBoardChoice,
					LWord &chosenBoard,
					Fl_Choice *hostNameChoice,
					bool &cardChangeRequired // True if cardChange()/switchCard() must be called on return
		);


	private:

	void removeConfiguration(Fl_Preferences &configFile, const char *configName );

	static const char *chosenBoardNumberKey;
	static const char *chosenBoardTypeKey;
	static const char *chosenBoardIDKey;
	static const char *chosenBoardDescriptionKey;
	static const char *chosenBoardHostnameKey;
	static const char *numHostnamesKey;
	static const char *hostnameItemKey;
	static const char *networkTimeoutKey;
};


#endif //NTV2FLTKNETPREFS_H








