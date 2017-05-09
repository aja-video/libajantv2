/**
	@file		persistence/persistence.h
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJAPersistence class.
**/

#ifndef AJAPersistence_H
#define AJAPersistence_H

#include <string>
#include <vector>
#include "ajabase/system/info.h"

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

    void SetParams(std::string appID="", std::string deviceType="", std::string deviceNumber="", bool bSharePrefFile=false);
    void GetParams(std::string& appID, std::string& deviceType, std::string& deviceNumber, bool& bSharePrefFile);

	bool SetValue(std::string key, void *value, AJAPersistenceType type, int blobBytes = 0);	
	bool GetValue(std::string key, void *value, AJAPersistenceType type, int blobBytes = 0);
	bool FileExists();
	bool DeletePrefFile();

    bool GetValuesInt(std::string key_query, std::vector<std::string>& keys, std::vector<int>& values);
    bool GetValuesBool(std::string key_query, std::vector<std::string>& keys, std::vector<bool>& values);
    bool GetValuesDouble(std::string key_query, std::vector<std::string>& keys, std::vector<double>& values);
    bool GetValuesString(std::string key_query, std::vector<std::string>& keys, std::vector<std::string>& values);
	
private:	

    std::string             mappId;
	std::string				mboardId;
	bool					mSharedPrefFile;
	std::string				mserialNumber;	
	std::string				mstateKeyName;

    AJASystemInfo           mSysInfo;
};

#endif	//	AJAPersistence_H
