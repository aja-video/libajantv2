/* SPDX-License-Identifier: MIT */
/**
	@file		persistence/persistence.h
	@brief		Declares the AJAPersistence class.
	@deprecated	This module will be removed from a future SDK.
	@copyright	(C) 2009-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJAPersistence_H
	#define AJAPersistence_H

	#include <string>
	#include <vector>
	#include "ajabase/system/info.h"


	/**
		@brief		Persistence data types.
		@deprecated	This enum will be removed from a future SDK.
	**/
	enum AJAPersistenceType
	{
		AJAPersistenceTypeInt,		
		AJAPersistenceTypeBool,
		AJAPersistenceTypeDouble,
		AJAPersistenceTypeString,	//	std::string (not C string)
		AJAPersistenceTypeBlob,
	
		//	Add any new ones above here
		AJAPersistenceTypeEnd
	};


	/**
		@brief		Class that talks to a device in such a way as to maintain a persistant state across apps & reboots.
		@deprecated	This class will be removed from a future SDK.
	**/
	class AJAPersistence
	{
		public: 
			NTV2_DEPRECATED_f(AJAPersistence());
			NTV2_DEPRECATED_f(AJAPersistence(const std::string& appID, const std::string& deviceType="", const std::string& deviceNumber="", bool bSharePrefFile=false));
			virtual			~AJAPersistence();

			NTV2_DEPRECATED_f(void SetParams(const std::string& appID="", const std::string& deviceType="", const std::string& deviceNumber="", bool bSharePrefFile=false));
			NTV2_DEPRECATED_f(void GetParams(std::string& appID, std::string& deviceType, std::string& deviceNumber, bool& bSharePrefFile));

			NTV2_DEPRECATED_f(bool SetValue(const std::string& key, const void *value, AJAPersistenceType type, size_t blobBytes = 0));
			NTV2_DEPRECATED_f(bool GetValue(const std::string& key, void *value, AJAPersistenceType type, size_t blobBytes = 0));
			NTV2_DEPRECATED_f(bool FileExists());
			NTV2_DEPRECATED_f(bool ClearPrefFile());
			NTV2_DEPRECATED_f(bool DeletePrefFile(bool makeBackup = false));
			NTV2_DEPRECATED_f(bool StorageHealthCheck(int& errCode, std::string& errMessage));

			NTV2_DEPRECATED_f(bool GetValuesInt(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<int>& values));
			NTV2_DEPRECATED_f(bool GetValuesBool(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<bool>& values));
			NTV2_DEPRECATED_f(bool GetValuesDouble(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<double>& values));
			NTV2_DEPRECATED_f(bool GetValuesString(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<std::string>& values));

			NTV2_DEPRECATED_f(void PathToPrefFile(std::string& path));

		public:
			//	Use these functions to focus read/write logging on specific keys
			static NTV2_DEPRECATED_f(bool	DebugLogAddKey		(const std::string & inKey));
			static NTV2_DEPRECATED_f(bool	DebugLogHasKeys		(void));
			static NTV2_DEPRECATED_f(bool	DebugLogHasKey		(const std::string & inKey));
			static NTV2_DEPRECATED_f(bool	DebugLogRemoveKey	(const std::string & inKey));
			static NTV2_DEPRECATED_f(bool	DebugLogRemoveAll	(void));

		private:
			std::string		mAppID;
			std::string		mBoardID;
			bool			mSharedPrefFile;
			std::string		mSerialNumber;	
			std::string		mStateKeyName;
			AJASystemInfo	mSysInfo;

	};	//	AJAPersistence

#endif	//	AJAPersistence_H
