/* SPDX-License-Identifier: MIT */
/**
	@file		persistence/persistence.h
	@brief		Declares the AJAPersistence class.
	@copyright	(C) 2009-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJAPersistence_H
	#define AJAPersistence_H

	#include <string>
	#include <vector>
	#include "ajabase/system/info.h"


	/**
	 * Persistence data types.
	 */
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
	 * Class used to talk to the board in such a way as to maintain a persistant state 
	 * across apps and reboots.
	 */
	class AJAPersistence
	{
		public: 
			AJAPersistence();
			AJAPersistence(const std::string& appID, const std::string& deviceType="", const std::string& deviceNumber="", bool bSharePrefFile=false);
			virtual ~AJAPersistence();

			void SetParams(const std::string& appID="", const std::string& deviceType="", const std::string& deviceNumber="", bool bSharePrefFile=false);
			void GetParams(std::string& appID, std::string& deviceType, std::string& deviceNumber, bool& bSharePrefFile);

			bool SetValue(const std::string& key, const void *value, AJAPersistenceType type, size_t blobBytes = 0);
			bool GetValue(const std::string& key, void *value, AJAPersistenceType type, size_t blobBytes = 0);
			bool FileExists();
			bool ClearPrefFile();
			bool DeletePrefFile(bool makeBackup = false);
			bool StorageHealthCheck(int& errCode, std::string& errMessage);

			bool GetValuesInt(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<int>& values);
			bool GetValuesBool(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<bool>& values);
			bool GetValuesDouble(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<double>& values);
			bool GetValuesString(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<std::string>& values);

			void PathToPrefFile(std::string& path);

		public:
			//	Use these functions to focus read/write logging on specific keys
			static bool DebugLogAddKey		(const std::string & inKey);
			static bool	DebugLogHasKeys		(void);
			static bool	DebugLogHasKey		(const std::string & inKey);
			static bool	DebugLogRemoveKey	(const std::string & inKey);
			static bool	DebugLogRemoveAll	(void);

		private:
			std::string		mAppID;
			std::string		mBoardID;
			bool			mSharedPrefFile;
			std::string		mSerialNumber;	
			std::string		mStateKeyName;
			AJASystemInfo	mSysInfo;

	};	//	AJAPersistence

#endif	//	AJAPersistence_H
