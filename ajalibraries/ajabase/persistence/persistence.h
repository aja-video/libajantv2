/**
	@file		persistence/persistence.h
	@copyright	Copyright (C) 2009-2016 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJAPersistence class.
**/

#ifndef AJAPersistence_H
#define AJAPersistence_H

#include <string>
#include <vector>

//Change these to match way linking in SDK

//#define USE_WITH_STREAMS			//If linking with Streams use this
//#define USE_WITH_NTV2    			//If linking with Ntv2 but not streams use this
//#define USE_WITH_NTV4				//If linking with Ntv4 but not streams use this

#if defined(USE_WITH_NTV2)
#include "ntv2status.h"
#endif

enum AJAPersistenceType
{
	AJAPersistenceTypeInt,		
	AJAPersistenceTypeBool,
	AJAPersistenceTypeDouble,
	AJAPersistenceTypeString,	//std::string not C string
	AJAPersistenceTypeBlob,
	
	//add any new ones above here
	AJAPersistenceTypeEnd
};


/**
 * Class used to talk to the board in such a way as to maintain a persistant state 
 * across apps and reboots.
 */
class AJAPersistence
{
public:	
	AJAPersistence();
	AJAPersistence(std::string appID, std::string deviceType="", std::string deviceNumber="", bool bSharePrefFile=false);
		
	virtual ~AJAPersistence();

	bool SetValue(std::string key, void *value, AJAPersistenceType type, int blobBytes = 0);	
	bool GetValue(std::string key, void *value, AJAPersistenceType type, int blobBytes = 0);
	bool FileExists();
	bool DeletePrefFile();

    bool GetValuesInt(std::string key_query, std::vector<std::string>& keys, std::vector<int>& values);
    bool GetValuesBool(std::string key_query, std::vector<std::string>& keys, std::vector<bool>& values);
    bool GetValuesDouble(std::string key_query, std::vector<std::string>& keys, std::vector<double>& values);
    bool GetValuesString(std::string key_query, std::vector<std::string>& keys, std::vector<std::string>& values);
	
	// for testing storage to disk
	bool UnitTestDiskReadWrite();	
	
#if defined(USE_WITH_STREAMS)
#endif
	
#if defined(USE_WITH_NTV2)	
	/**
	 * Constructor that allows immediate binding to a card
	 *	@param card (in) Pointer to a card to use when changing persist state.
	 */
	AJAPersistence(CNTV2Status *card);

	/**
	 * Bindings to a specific card.
	 *	@param card (in) Pointer to a card to use when changing persist state.
	 */	
	bool BindToCard(CNTV2Status *card);	
#endif

#if defined(USE_WITH_NTV4)
#endif
	
private:
	void Init();
	
#if defined(USE_WITH_NTV2)	
	CNTV2Status 	*mpboardHandle;
#endif	
	
	std::string				mboardId;
	bool					mSharedPrefFile;
	std::string				mserialNumber;	
	std::string				mstateKeyName;
};

#endif	//	AJAPersistence_H
