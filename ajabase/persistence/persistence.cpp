/* SPDX-License-Identifier: MIT */
/**
	@file		persistence/persistence.cpp
	@brief		Implements the AJAPersistence class.
	@copyright	(C) 2009-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "persistence.h"
#include "sqlite3.h"
#include "stdlib.h"

#include <iostream>
#include <cstdio>
#include <ctime>

#include "ajabase/common/common.h"
#include "ajabase/system/debug.h"
#include "ajabase/system/file_io.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/lock.h"

// Mac defines
#if !defined(AJA_MAC) && defined(AJAMac)
	#define AJA_MAC
#endif

// Linux defines
#if !defined(AJA_LINUX) && defined(AJALinux)
	#define AJA_LINUX
#endif

// Windows defines
#if !defined(AJA_WINDOWS) && defined(MSWindows)
	#define AJA_WINDOWS
#endif

// Platform includes
#if defined(AJA_MAC)
	#include <sys/stat.h>
	#include <unistd.h>
	#define AJAFUNC		__func__
#elif defined(AJA_WINDOWS)
	#include <Windows.h>
	#include <io.h>
	#pragma warning(disable:4996)
	#ifndef F_OK
		#define F_OK 0
	#endif
	#define AJAFUNC		__FUNCTION__
#elif defined(AJA_LINUX)
	#include <sys/stat.h>
	#include <stdio.h>
	#include <string.h>
	#include <unistd.h>
	#ifndef F_OK
		#define F_OK 0
	#endif
	#define AJAFUNC		__func__
#endif

static std::vector<std::string>	sTypeLabelsVector;
static AJALock					sTypeLabelsVectorLock;
static std::set<std::string>	sLogKeys;
static AJALock					sLogKeysLock;

#define addTypeLabelToVector(_x_)	sTypeLabelsVector.push_back(#_x_)
#define addTypeLabel(_x_)			sTypeLabelsVector.push_back(_x_)

static inline void initTypeLabels (void)
{
	AJAAutoLock autoLock(&sTypeLabelsVectorLock);
	sTypeLabelsVector.clear();
	addTypeLabel("int");	//	addTypeLabelToVector(AJAPersistenceTypeInt);
	addTypeLabel("bool");	//	addTypeLabelToVector(AJAPersistenceTypeBool);
	addTypeLabel("double");	//	addTypeLabelToVector(AJAPersistenceTypeDouble);
	addTypeLabel("string");	//	addTypeLabelToVector(AJAPersistenceTypeString);
	addTypeLabel("blob");	//	addTypeLabelToVector(AJAPersistenceTypeBlob);
	addTypeLabel("end");	//	addTypeLabelToVector(AJAPersistenceTypeEnd);
}

static inline std::string labelForPersistenceType (AJAPersistenceType type)
{
	AJAAutoLock autoLock(&sTypeLabelsVectorLock);
	if (type >= 0  &&  type < AJAPersistenceTypeEnd	 &&	 size_t(type) < sTypeLabelsVector.size())
		return sTypeLabelsVector.at(type);
	else
		return "unk";
}

bool AJAPersistence::DebugLogAddKey (const std::string & inKey)
{
	AJAAutoLock lock(&sLogKeysLock);
	if (sLogKeys.find(inKey) != sLogKeys.end())
		return false;
	sLogKeys.insert(inKey);
	return true;
}

bool AJAPersistence::DebugLogHasKeys (void)
{
	AJAAutoLock lock(&sLogKeysLock);
	return !sLogKeys.empty();
}

bool AJAPersistence::DebugLogHasKey (const std::string & inKey)
{
	AJAAutoLock lock(&sLogKeysLock);
	std::set<std::string>::iterator it(sLogKeys.find(inKey));
	return it != sLogKeys.end();
}

bool AJAPersistence::DebugLogRemoveKey (const std::string & inKey)
{
	AJAAutoLock lock(&sLogKeysLock);
	std::set<std::string>::iterator it(sLogKeys.find(inKey));
	if (it == sLogKeys.end())
		return false;
	sLogKeys.erase(it);
	return true;
}

bool AJAPersistence::DebugLogRemoveAll (void)
{
	AJAAutoLock lock(&sLogKeysLock);
	sLogKeys.clear();
	return true;
}

// Reduce the typing when using the logging macros
//#define AJA_VERBOSE_LOGGING = 1
#define AJA_LOG_DEBUG(_should_log_, _xpr_)		do (if (_should_log_) AJA_sDEBUG(AJA_DebugUnit_Persistence,	AJAFUNC << ": " << _xpr_); } while(false)
#define AJA_LOG_INFO(_should_log_, _xpr_)		do {if (_should_log_) AJA_sINFO(AJA_DebugUnit_Persistence,	AJAFUNC << ": " << _xpr_); } while(false)
#define AJA_LOG_NOTICE(_should_log_, _xpr_)		do {if (_should_log_) AJA_sNOTICE(AJA_DebugUnit_Persistence,AJAFUNC << ": " << _xpr_); } while(false)
#define AJA_LOG_WARN(_should_log_, _xpr_)		do {if (_should_log_) AJA_sWARNING(AJA_DebugUnit_Persistence,AJAFUNC << ": " << _xpr_); } while(false)
#define AJA_LOG_ERROR(_should_log_, _xpr_)		do {if (_should_log_) AJA_sERROR(AJA_DebugUnit_Persistence,	AJAFUNC << ": " << _xpr_); } while(false)
//#define AJA_LOG_ALERT(_should_log_, _xpr_)		do {if (_should_log_) AJA_sALERT(AJA_DebugUnit_Persistence,		AJAFUNC << ": " << _xpr_); } while(false)
//#define AJA_LOG_EMERGENCY(_should_log_, _xpr_)	do {if (_should_log_) AJA_sEMERGENCY(AJA_DebugUnit_Persistence,	AJAFUNC << ": " << _xpr_); } while(false)
//#define AJA_LOG_ASSERT(_should_log_, _xpr_)		do {if (_should_log_) AJA_sASSERT(AJA_DebugUnit_Persistence,	AJAFUNC << ": " << _xpr_); } while(false)

// Use different log levels so can better sort reads/writes
#define AJA_LOG_READ(_should_log_, _xpr_)	   AJA_LOG_INFO(_should_log_, _xpr_)
#define AJA_LOG_WRITE(_should_log_, _xpr_)	   AJA_LOG_NOTICE(_should_log_, _xpr_)

inline bool should_we_log()
{
	int32_t refCount = 0;
	AJADebug::GetClientReferenceCount(&refCount);
	return refCount > 0;
}

// There are a few places where we want to try calling sqlite3_*() calls a couple of times if busy
// this macro wraps that and provides logging
#define RETRY_SQLITE_CALL_WITH_LOGGING(_should_log_, _result_code_, _num_max_retries_, _sleep_time_in_microsec_,\
									   _retry_condition_, _message_, _sqlite_function_call_)\
{\
	int last_i = _num_max_retries_ - 1;\
	for(int i=0;i<_num_max_retries_;i++)\
	{\
		_result_code_ = _sqlite_function_call_;\
		if (_retry_condition_)\
		{\
			if (_should_log_)\
			{\
				std::ostringstream oss;\
				oss << "sqlite> attempt: " << i+1 << " of " << _num_max_retries_ << ", error code: " << _result_code_ <<\
					   " with message: \"" << _message_;\
				if (i == last_i)\
					{ AJA_LOG_ERROR(_should_log_, oss.str()); }\
				else\
					{ AJA_LOG_WARN(_should_log_, oss.str()); }\
			}\
			if (i != last_i)\
				AJATime::SleepInMicroseconds(_sleep_time_in_microsec_);\
		}\
		else\
			break;\
	}\
}

// Encapsulate the sqlite3_stmt object so automatically handled by constructor/destructor
const int gDefaultNumSqliteRetries = 6;
const int32_t gDefaultMicrosecondsBetweenRetries = 2500;//2000;

// Encapsulate the sqlite3 object so automatically handled by constructor/destructor
class AJAPersistenceDBImplObject
{
public:
		AJAPersistenceDBImplObject(const std::string &pathToDB)
		: mDb(NULL), mPath(pathToDB), mOpenErrorCode(SQLITE_ERROR)
		{
			mOpenErrorCode = sqlite3_open_v2(mPath.c_str(), &mDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
			bool shouldLog = should_we_log();
			if (mOpenErrorCode != SQLITE_OK)
			{
				AJA_LOG_ERROR(shouldLog, "sqlite> error code: " << mOpenErrorCode <<
										 " with message: \"" << sqlite3_errmsg(mDb) << "\" when opening DB at: " << mPath);
			}
			else
			{
#if AJA_VERBOSE_LOGGING
				AJA_LOG_WRITE(shouldLog, "sqlite> successfully opened handle to DB at: " << mPath);
#endif
				mOpenErrorCode = Execute("PRAGMA journal_mode=DELETE;");
			}
		}

		virtual ~AJAPersistenceDBImplObject()
		{
			int rc = sqlite3_close(mDb);
			bool shouldLog = should_we_log();
			if (rc != SQLITE_OK)
			{
				AJA_LOG_ERROR(shouldLog, "sqlite> error code: " << rc <<
										 " with message: \"" << sqlite3_errmsg(mDb) << "\" when closing DB at: " << mPath);
			}
			else
			{
#if AJA_VERBOSE_LOGGING
				AJA_LOG_WRITE(shouldLog, "sqlite> successfully closed handle to DB at: " << mPath);
#endif
			}
		}

		bool IsDBOpen()			{ return (mOpenErrorCode == SQLITE_OK && mDb); }
		int OpenErrorCode()		{ return mOpenErrorCode; }
		std::string PathToDB()	{ return mPath; }
		sqlite3* GetHandle()	{ return mDb; }

		int Execute(const std::string& stmt,
					int numRetries = gDefaultNumSqliteRetries, int sleepBetweenTries = gDefaultMicrosecondsBetweenRetries)
		{
			if (IsDBOpen())
			{
				int rc = SQLITE_ERROR;
				bool shouldLog = should_we_log();
				RETRY_SQLITE_CALL_WITH_LOGGING(shouldLog, rc, numRetries, sleepBetweenTries,
											   rc != SQLITE_OK, sqlite3_errstr(rc) << "\" when executing statement: " << stmt,
											   sqlite3_exec(mDb, stmt.c_str(), NULL, NULL, NULL));
				return rc;
			}
			else
				return SQLITE_CANTOPEN;
		}

		int Checkpoint(const std::string& dbName, int checkpointMode = SQLITE_CHECKPOINT_PASSIVE)
		{
			if (IsDBOpen())
				return sqlite3_wal_checkpoint_v2(mDb, dbName.c_str(), checkpointMode, NULL, NULL);
			else
				return SQLITE_ERROR;
		}

private:
		sqlite3 *mDb;
		std::string mPath;
		int mOpenErrorCode;
};

class AJAPersistenceDBImplStatement
{
public:
		AJAPersistenceDBImplStatement()
		: mDb(NULL), mStmt(NULL), mStmtString(""), mPrepareErrorCode(SQLITE_ERROR),
		  mNumRetries(gDefaultNumSqliteRetries), mMicrosecondsBetweenRetries(gDefaultMicrosecondsBetweenRetries)
		{
		}

		AJAPersistenceDBImplStatement(AJAPersistenceDBImplObject &db, const std::string &stmt)
		: mDb(&db), mStmt(NULL), mStmtString(""), mPrepareErrorCode(SQLITE_ERROR),
		  mNumRetries(gDefaultNumSqliteRetries), mMicrosecondsBetweenRetries(gDefaultMicrosecondsBetweenRetries)
		{
			Prepare(stmt);
		}

		void SetDb(AJAPersistenceDBImplObject &db)
		{
			mDb = &db;
		}

		int Prepare(const std::string &stmt)
		{
			bool shouldLog = should_we_log();

			mStmtString = stmt;

			if (mDb == NULL || mDb->GetHandle() == NULL)
			{
				AJA_LOG_ERROR(shouldLog, "sqlite> DB handle invalid for path: \"" << mDb->PathToDB() << "\", could not prepare statement: " << mStmtString);
				return SQLITE_ERROR;
			}

			mPrepareErrorCode = SQLITE_ERROR;
			RETRY_SQLITE_CALL_WITH_LOGGING(shouldLog, mPrepareErrorCode, mNumRetries, mMicrosecondsBetweenRetries,
										   mPrepareErrorCode != SQLITE_OK, sqlite3_errmsg(mDb->GetHandle()) << "\" when preparing statement: " << mStmtString,
										   sqlite3_prepare_v3(mDb->GetHandle(), mStmtString.c_str(), -1, SQLITE_PREPARE_PERSISTENT, &mStmt, NULL));
			return mPrepareErrorCode;
		}

		virtual ~AJAPersistenceDBImplStatement()
		{
			sqlite3_finalize(mStmt);
		}

		int Reset()
		{
			if (mStmt == NULL)
			{
				AJA_LOG_ERROR(should_we_log(), "sqlite> could not reset, statement handle invalid for statement: " << mStmtString);
				return SQLITE_ERROR;
			}

			int rc = sqlite3_reset(mStmt);
			return rc;
		}

		int BindText(int parameterNum, const std::string& value)
		{
			if (mStmt == NULL)
			{
				AJA_LOG_ERROR(should_we_log(), "sqlite> could not bind text, statement handle invalid for statement: " << mStmtString);
				return SQLITE_ERROR;
			}

			int rc = sqlite3_bind_text(mStmt, parameterNum, value.c_str(), -1, NULL);
			return rc;
		}

		int BindBlob(int parameterNum, int blobSizeInBytes, const void *value)
		{
			if (mStmt == NULL)
			{
				AJA_LOG_ERROR(should_we_log(), "sqlite> could not bind blob, statement handle invalid for statement: " << mStmtString);
				return SQLITE_ERROR;
			}

			int rc = sqlite3_bind_blob(mStmt, parameterNum, value, blobSizeInBytes, SQLITE_TRANSIENT);
			return rc;
		}

		int Step()
		{
			bool shouldLog = should_we_log();
			if (mDb == NULL)
			{
				AJA_LOG_ERROR(shouldLog, "sqlite> could not step, DB handle invalid");
				return SQLITE_ERROR;
			}

			if (mDb->IsDBOpen() == false)
			{
				AJA_LOG_ERROR(shouldLog, "sqlite> could not step, DB not opened at: " << mDb->PathToDB());
				return SQLITE_ERROR;
			}

			if (mStmt == NULL)
			{
				AJA_LOG_ERROR(shouldLog, "sqlite> could not step, statement handle invalid for statement: " << mStmtString);
				return SQLITE_ERROR;
			}			

			int rc = SQLITE_ERROR;
			RETRY_SQLITE_CALL_WITH_LOGGING(shouldLog, rc, mNumRetries, mMicrosecondsBetweenRetries,
										   rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE,
										   sqlite3_errstr(rc) << "\" when stepping statement: " << sqlite3_expanded_sql(mStmt),
										   sqlite3_step(mStmt));
			return rc;
		}

		std::string ColumnText(int col)
		{
			if (mStmt == NULL)
			{
				AJA_LOG_ERROR(should_we_log(), "sqlite> could not get column text, statement handle invalid for statement: " << mStmtString);
				return "";
			}

			return std::string((const char*)sqlite3_column_text(mStmt, col));
		}

		bool ColumnBlob(int col, void* output, int& blobSize)
		{
			if (mStmt == NULL)
			{
				AJA_LOG_ERROR(should_we_log(), "sqlite> could not get column blob, statement handle invalid for statement: " << mStmtString);
				return false;
			}

			blobSize = sqlite3_column_bytes(mStmt, col);
			if (blobSize > 0)
				memcpy(output, sqlite3_column_blob(mStmt, col), blobSize);

			return (blobSize > 0);
		}

		int PrepareErrorCode()
		{
			return mPrepareErrorCode;
		}

		std::string GetString()
		{
			return mStmtString;
		}

private:
		AJAPersistenceDBImplObject *mDb;
		sqlite3_stmt *mStmt;
		std::string mStmtString;
		int mPrepareErrorCode;
		int mNumRetries;
		int32_t mMicrosecondsBetweenRetries;
};

// This is the class that AJAPersistence uses directly to communicate with the SQLite layer
class AJAPersistenceDBImpl
{
public:
		AJAPersistenceDBImpl(const std::string &pathToDB)
		: mDb(pathToDB)
		{
			if (IsDBOpen())
			{				 
				// generic table statements
				AJAPersistenceDBImplStatement createTableStmt(mDb, "CREATE TABLE IF NOT EXISTS persistence(id INTEGER, name CHAR(255), value CHAR(64), dev_name CHAR(64), dev_num CHAR(64), PRIMARY KEY(id))");
				AJAPersistenceDBImplStatement blobCreateTableStmt(mDb, "CREATE TABLE IF NOT EXISTS persistenceBlobs(id INTEGER, name CHAR(255), value BLOB, dev_name CHAR(64), dev_num CHAR(64), PRIMARY KEY(id))");

				// The tables must exist before the other prepared statements can be made
				mTableCreateErrorCode = createTableStmt.Step();
				mTableCreateBlobErrorCode = blobCreateTableStmt.Step();

				// normal table statements
				mGetValueSpecificStmt.SetDb(mDb);
				mGetValueLessSpecificStmt.SetDb(mDb);
				mInsertOrReplaceStmt.SetDb(mDb);
				mGetValueSpecificStmt.Prepare("SELECT value FROM persistence WHERE name=?1 AND dev_name=?2 AND dev_num=?3");
				mGetValueLessSpecificStmt.Prepare("SELECT value FROM persistence WHERE name=?1 AND dev_name=?2");
				mInsertOrReplaceStmt.Prepare("INSERT OR REPLACE INTO persistence (id, name, value, dev_name, dev_num) SELECT id, ?1, ?2, ?3, ?4 "
											 "FROM ( SELECT NULL ) LEFT JOIN ( SELECT * FROM persistence WHERE name=?5 AND dev_name=?6 AND dev_num=?7)");

				// blob table statements
				mBlobGetValueSpecificStmt.SetDb(mDb);
				mBlobGetValueLessSpecificStmt.SetDb(mDb);
				mBlobInsertOrReplaceStmt.SetDb(mDb);
				mBlobGetValueSpecificStmt.Prepare("SELECT value FROM persistenceBlobs WHERE name=?1 AND dev_name=?2 AND dev_num=?3");
				mBlobGetValueLessSpecificStmt.Prepare("SELECT value FROM persistenceBlobs WHERE name=?1 AND dev_name=?2");
				mBlobInsertOrReplaceStmt.Prepare("INSERT OR REPLACE INTO persistenceBlobs (id, name, value, dev_name, dev_num) SELECT id, ?1, ?2, ?3, ?4 "
												 "FROM ( SELECT NULL ) LEFT JOIN ( SELECT * FROM persistenceBlobs WHERE name=?5 AND dev_name=?6 AND dev_num=?7)");

				// multiple return statements
				mGetAllValuesSpecificStmt.SetDb(mDb);
				mGetAllValuesLessSpecificStmt.SetDb(mDb);
				mGetAllValuesSpecificStmt.Prepare("SELECT name, value FROM persistence WHERE name LIKE ?1 AND dev_name=?2 AND dev_num=?3");
				mGetAllValuesLessSpecificStmt.Prepare("SELECT name, value FROM persistence WHERE name LIKE ?1 AND dev_name=?2");
			}
			else
			{
				AJA_LOG_ERROR(should_we_log(), "sqlite> could not prepare statements DB not opened at: " << mDb.PathToDB());
			}
		}

		virtual ~AJAPersistenceDBImpl()
		{
		}

		bool ClearTables()
		{
			int rc = mDb.Execute("DELETE FROM persistence; DELETE FROM persistenceBlobs;");
			return (rc == SQLITE_OK || rc == SQLITE_DONE);
		}

		static bool ConvertStringToValueType(const std::string& inputString, AJAPersistenceType type, void *outputValue)
		{
			bool isGood = false;
			switch(type)
			{
				case AJAPersistenceTypeInt:
				{
					int *outVal = (int*)outputValue;
					*outVal = atoi(inputString.c_str());
					isGood = true;
				}
				break;

				case AJAPersistenceTypeBool:
				{
					bool *outVal = (bool*)outputValue;
					int inVal = atoi(inputString.c_str());
					*outVal = (inVal == 1) ? true : false;
					isGood = true;
				}
				break;

				case AJAPersistenceTypeDouble:
				{
					double *outVal = (double*)outputValue;
					*outVal = atof(inputString.c_str());
					isGood = true;
				}
				break;

				case AJAPersistenceTypeString:
				{
					std::string *valuePtr = (std::string *)outputValue;
					valuePtr->erase();
					valuePtr->append(inputString.c_str());
					isGood = true;
				}
				break;

				default:
					isGood = false;
					break;
			}

			return isGood;
		}

		static bool ConvertValueTypeToString(const void *inputValue, AJAPersistenceType type, std::string& outputString)
		{
			bool isGood = false;
			switch(type)
			{
				case AJAPersistenceTypeInt:
				{
					outputString = aja::to_string(*((const int*)inputValue));
					isGood = true;
				}
				break;

				case AJAPersistenceTypeBool:
				{
					outputString = (*((const bool*)inputValue) == true) ? "1" : "0";
					isGood = true;
				}
				break;

				case AJAPersistenceTypeDouble:
				{
					outputString = aja::to_string(*((const double*)inputValue));
					isGood = true;
				}
				break;

				case AJAPersistenceTypeString:
				{
					outputString = *((const std::string *)inputValue);
					isGood = true;
				}
				break;

				default:
					isGood = false;
					break;
			}

			return isGood;
		}

		bool GetValue(std::string keyQuery, void *value, AJAPersistenceType type, int blobSizeInBytes, std::string devName = "", std::string devNum = "")
		{
			bool isGood = false;
			if (IsDBOpen())
			{
				AJAPersistenceDBImplStatement *specificStmt;
				AJAPersistenceDBImplStatement *lessSpecificStmt;

				if (type == AJAPersistenceTypeBlob)
				{
					specificStmt = &mBlobGetValueSpecificStmt;
					lessSpecificStmt = &mBlobGetValueLessSpecificStmt;
				}
				else
				{
					specificStmt = &mGetValueSpecificStmt;
					lessSpecificStmt = &mGetValueLessSpecificStmt;
				}

				// Reset statements and bind parameters
				specificStmt->Reset();
				lessSpecificStmt->Reset();

				specificStmt->BindText(1, keyQuery);
				specificStmt->BindText(2, devName);
				specificStmt->BindText(3, devNum);

				lessSpecificStmt->BindText(1, keyQuery);
				lessSpecificStmt->BindText(2, devName);

				// get first row results
				if (specificStmt->Step() == SQLITE_ROW)
				{
					if (type == AJAPersistenceTypeBlob)
					{
						int blobSize = 0;
						isGood = specificStmt->ColumnBlob(0, value, blobSize);
						if (blobSize > blobSizeInBytes)
							isGood = false;
					}
					else
					{
						isGood = ConvertStringToValueType(specificStmt->ColumnText(0), type, value);
					}
				}
				else if (lessSpecificStmt->Step() == SQLITE_ROW)
				{
					if (type == AJAPersistenceTypeBlob)
					{
						int blobSize = 0;
						isGood = lessSpecificStmt->ColumnBlob(0, value, blobSize);
						if (blobSize > blobSizeInBytes)
							isGood = false;
					}
					else
					{
						isGood = ConvertStringToValueType(lessSpecificStmt->ColumnText(0), type, value);
					}
				}
			}

			return isGood;
		}

		bool SetValue(std::string keyQuery, const void *value, AJAPersistenceType type, int blobSizeInBytes, std::string devName = "", std::string devNum = "")
		{
			bool isGood = false;
			if (IsDBOpen())
			{
				AJAPersistenceDBImplStatement *insertOrReplaceStmt;

				std::string strValue;
				bool goodToSet = false;
				if (type == AJAPersistenceTypeBlob)
				{
					insertOrReplaceStmt = &mBlobInsertOrReplaceStmt;
					goodToSet			= true;
				}
				else
				{
					insertOrReplaceStmt = &mInsertOrReplaceStmt;
					goodToSet			= ConvertValueTypeToString(value, type, strValue);
				}

				if (goodToSet)
				{
					// Reset statement and bind parameters
					insertOrReplaceStmt->Reset();

					if (type == AJAPersistenceTypeBlob)
					{
						insertOrReplaceStmt->BindText(1, keyQuery);
						insertOrReplaceStmt->BindBlob(2, blobSizeInBytes, value);
						insertOrReplaceStmt->BindText(3, devName);
						insertOrReplaceStmt->BindText(4, devNum);
						insertOrReplaceStmt->BindText(5, keyQuery);
						insertOrReplaceStmt->BindText(6, devName);
						insertOrReplaceStmt->BindText(7, devNum);
					}
					else
					{
						insertOrReplaceStmt->BindText(1, keyQuery);
						insertOrReplaceStmt->BindText(2, strValue);
						insertOrReplaceStmt->BindText(3, devName);
						insertOrReplaceStmt->BindText(4, devNum);
						insertOrReplaceStmt->BindText(5, keyQuery);
						insertOrReplaceStmt->BindText(6, devName);
						insertOrReplaceStmt->BindText(7, devNum);
					}

					if (insertOrReplaceStmt->Step() == SQLITE_DONE)
					{
						// update was good
						isGood = true;
						/*int checkpointMode = SQLITE_CHECKPOINT_FULL;
						if (type == AJAPersistenceTypeBlob)
							mDb.Checkpoint("persistenceBlobs", checkpointMode);
						else
							mDb.Checkpoint("persistence", checkpointMode);*/
					}
				}
			}
			return isGood;
		}

		bool GetAllMatchingValues(std::string keyQuery, std::vector<std::string>& keys, std::vector<std::string>& values,
								  std::string devName = "", std::string devNum = "")
		{
			bool isGood = false;
			if (IsDBOpen())
			{
				// Reset statements and bind parameters
				mGetAllValuesSpecificStmt.Reset();
				mGetAllValuesLessSpecificStmt.Reset();

				mGetAllValuesSpecificStmt.BindText(1, keyQuery);
				mGetAllValuesSpecificStmt.BindText(2, devName);
				mGetAllValuesSpecificStmt.BindText(3, devNum);

				mGetAllValuesLessSpecificStmt.BindText(1, keyQuery);
				mGetAllValuesLessSpecificStmt.BindText(2, devName);

				int ret_code = mGetAllValuesSpecificStmt.Step();
				while(ret_code == SQLITE_ROW)
				{
					std::string name = mGetAllValuesSpecificStmt.ColumnText(0);
					std::string value = mGetAllValuesSpecificStmt.ColumnText(1);
					keys.push_back(name);
					values.push_back(value);

					ret_code = mGetAllValuesSpecificStmt.Step();
				}

				if (keys.empty())
				{
					ret_code = mGetAllValuesLessSpecificStmt.Step();
					while(ret_code == SQLITE_ROW)
					{
						std::string name = mGetAllValuesLessSpecificStmt.ColumnText(0);
						std::string value = mGetAllValuesLessSpecificStmt.ColumnText(1);
						keys.push_back(name);
						values.push_back(value);

						ret_code = mGetAllValuesLessSpecificStmt.Step();
					}
				}

				if (keys.empty() == false)
				{
					isGood = true;
				}
			}
			return isGood;
		}

		bool IsDBOpen()	{ return mDb.IsDBOpen(); }

		int OpenErrorCode() { return mDb.OpenErrorCode(); }

private:
	  AJAPersistenceDBImplObject	mDb;
	  int							mTableCreateErrorCode;
	  int							mTableCreateBlobErrorCode;

	  // normal statements
	  AJAPersistenceDBImplStatement mGetValueSpecificStmt;
	  AJAPersistenceDBImplStatement mGetValueLessSpecificStmt;
	  AJAPersistenceDBImplStatement mInsertOrReplaceStmt;

	  // blob statements
	  AJAPersistenceDBImplStatement mBlobGetValueSpecificStmt;
	  AJAPersistenceDBImplStatement mBlobGetValueLessSpecificStmt;
	  AJAPersistenceDBImplStatement mBlobInsertOrReplaceStmt;

	  // multiple return statements
	  AJAPersistenceDBImplStatement mGetAllValuesSpecificStmt;
	  AJAPersistenceDBImplStatement mGetAllValuesLessSpecificStmt;
};

// Start of Public Class AJAPersistence

AJAPersistence::AJAPersistence()
	: mSysInfo(AJA_SystemInfoMemoryUnit_Megabytes, AJA_SystemInfoSection_Path)
{
	initTypeLabels();
	SetParams("null_device");
}

AJAPersistence::AJAPersistence(const std::string& appID, const std::string& deviceType, const std::string& deviceNumber, bool bSharePrefFile)
{
	initTypeLabels();
	SetParams(appID, deviceType, deviceNumber, bSharePrefFile);
}

AJAPersistence::~AJAPersistence()
{
}

void AJAPersistence::SetParams(const std::string& appID, const std::string& deviceType, const std::string& deviceNumber, bool bSharePrefFile)
{
	std::string lastStateKeyName = mStateKeyName;
	mAppID			= appID;
	mBoardID		= deviceType;
	mSerialNumber	= deviceNumber;
	mSharedPrefFile = bSharePrefFile;
	mSysInfo.GetValue(mSharedPrefFile ? AJA_SystemInfoTag_Path_PersistenceStoreSystem : AJA_SystemInfoTag_Path_PersistenceStoreUser, mStateKeyName);
	mStateKeyName += appID;

	bool shouldLog = should_we_log();
	if (mAppID != "null_device")
		AJA_LOG_INFO(shouldLog, mStateKeyName << ": dev='" << mBoardID << "' devNum='" << mSerialNumber << "'");
}

void AJAPersistence::GetParams(std::string& appID, std::string& deviceType, std::string& deviceNumber, bool& bSharePrefFile)
{
	appID = mAppID;
	deviceType = mBoardID;
	deviceNumber = mSerialNumber;
	bSharePrefFile = mSharedPrefFile;
}

bool AJAPersistence::SetValue(const std::string& key, const void *value, AJAPersistenceType type, size_t blobSize)
{
	bool shouldLog(should_we_log()), isGood(false);
	int dbOpenErr = 0;
	{
		AJAPersistenceDBImpl db(mStateKeyName);
		if (db.IsDBOpen())
			isGood = db.SetValue(key, value, type, int(blobSize), mBoardID, mSerialNumber);
		else
			dbOpenErr = db.OpenErrorCode();
	}

	if (shouldLog)
	{
		std::string dbgValue("cannotLogValue"), dbOpenErrStr;
		AJAPersistenceDBImpl::ConvertValueTypeToString(value, type, dbgValue);
		if (dbOpenErr)
			dbOpenErrStr = "(" + aja::to_string(dbOpenErr) + ")";
		if (!DebugLogHasKeys() ||  DebugLogHasKey(key))
			AJA_LOG_WRITE(shouldLog, mStateKeyName << dbOpenErrStr << ": '" << mBoardID << "' " << mSerialNumber << ": "
									<< " key='" << key << "' " << labelForPersistenceType(type) << " val='" << dbgValue << "'");
	}
	return isGood;
}

bool AJAPersistence::GetValue(const std::string& key, void *value, AJAPersistenceType type, size_t blobSize)
{
	// with Get, don't create file if it does not exist
	if (!FileExists())
		return false;

	bool shouldLog(should_we_log()), isGood(false);
	int dbOpenErr = 0;
	{
		AJAPersistenceDBImpl db(mStateKeyName);
		if (db.IsDBOpen())
			isGood = db.GetValue(key, value, type, int(blobSize), mBoardID, mSerialNumber);
		else
			dbOpenErr = db.OpenErrorCode();
	}

	if (shouldLog)
	{
		std::string dbgValue("cannotLogValue"), dbOpenErrStr;
		AJAPersistenceDBImpl::ConvertValueTypeToString(value, type, dbgValue);
		if (dbOpenErr)
			dbOpenErrStr = "(" + aja::to_string(dbOpenErr) + ")";
		if (!DebugLogHasKeys() ||  DebugLogHasKey(key))
			AJA_LOG_READ(shouldLog, mStateKeyName << dbOpenErrStr << ": '" << mBoardID << "' " << mSerialNumber << ": "
									<< " key='" << key << "' " << labelForPersistenceType(type) << " val='" << dbgValue << "'");
	}
	return isGood;
}

bool AJAPersistence::GetValuesString(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<std::string>& values)
{
	// with Get, don't create file if it does not exist
	if (!FileExists())
		return false;

	bool shouldLog(should_we_log()), isGood(false);
	int dbOpenErr = 0;
	if (!shouldLog)
		for (std::vector<std::string>::const_iterator it(keys.begin());  it != keys.end();  ++it)
			if (DebugLogHasKey(*it))
				{shouldLog = true;  break;}
	{
		AJAPersistenceDBImpl db(mStateKeyName);
		if (db.IsDBOpen())
			isGood = db.GetAllMatchingValues(keyQuery, keys, values, mBoardID, mSerialNumber);
		else
			dbOpenErr = db.OpenErrorCode();
	}
	if (shouldLog)
	{
		std::string dbOpenErrStr;
		if (dbOpenErr)
			dbOpenErrStr = "(" + aja::to_string(dbOpenErr) + ")";
		AJA_LOG_READ(shouldLog, mStateKeyName << dbOpenErrStr << ": query='" << keyQuery << "'");
	}
	return isGood;
}

bool AJAPersistence::GetValuesInt(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<int>& values)
{
	// with Get, don't create file if it does not exist
	if (!FileExists())
		return false;

	AJA_LOG_READ(should_we_log(), mStateKeyName << ": query='" << keyQuery << "'");
	std::vector<std::string> tmpValues;
	if (!GetValuesString(keyQuery, keys, tmpValues))
		return false;
	for (size_t i(0);  i < keys.size();  i++)
		values.push_back(atoi(tmpValues.at(i).c_str()));
	return true;
}

bool AJAPersistence::GetValuesBool(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<bool>& values)
{
	// with Get, don't create file if it does not exist
	if (!FileExists())
		return false;

	AJA_LOG_READ(should_we_log(), mStateKeyName << ": query='" << keyQuery << "'");
	std::vector<std::string> tmpValues;
	if (!GetValuesString(keyQuery, keys, tmpValues))
		return false;
	for (size_t i(0);  i < keys.size();  i++)
		values.push_back((atoi(tmpValues.at(i).c_str()) == 1) ? true : false);
	return true;
}

bool AJAPersistence::GetValuesDouble(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<double>& values)
{
	// with Get, don't create file if it does not exist
	if (!FileExists())
		return false;

	AJA_LOG_READ(should_we_log(), mStateKeyName << ": query='" << keyQuery << "'");
	std::vector<std::string> tmpValues;
	if (!GetValuesString(keyQuery, keys, tmpValues))
		return false;
	for (int i(0);  i < int(keys.size());  i++)
		values.push_back(atof(tmpValues.at(i).c_str()));
	return true;
}

void AJAPersistence::PathToPrefFile(std::string& path)
{
	path = mStateKeyName;
}

bool AJAPersistence::FileExists()
{
	return AJAFileIO::FileExists(mStateKeyName.c_str());
}

bool AJAPersistence::ClearPrefFile()
{
	bool shouldLog = should_we_log();
	bool bSuccess = true;
	if (FileExists())
	{
		AJAPersistenceDBImpl db(mStateKeyName);
		AJA_LOG_INFO(shouldLog, "cleared existing db tables");
		bSuccess = db.ClearTables();
	}
	else
		AJA_LOG_WARN(shouldLog, "failed, file not found");
	return bSuccess;
}

bool backup_pref_file(const std::string &path, bool shouldLog)
{
	bool result = false;
	if (AJAFileIO::FileExists(path.c_str()))
	{
		time_t t = std::time(NULL);
		if(t != (time_t)(-1))
		{
			static char dateBuffer[64];

			struct tm *timeinfo = std::localtime(&t);
			std::strftime(dateBuffer, sizeof(dateBuffer), "_%Y_%m_%d_%H%M%S", timeinfo);

			std::string newPath = path;
			newPath += dateBuffer;

			result = std::rename(path.c_str(), newPath.c_str()) == 0 ? true : false;
			if (result)
				AJA_LOG_NOTICE(shouldLog, "db file backed up, from '" << path << "' to '" << newPath << "'");
			else
				AJA_LOG_WARN(shouldLog, "db file backup failed, from '" << path << "' to '" << newPath << "'");
		}
	}
	return result;
}

// delete pref file
bool AJAPersistence::DeletePrefFile(bool makeBackup)
{
	bool shouldLog = should_we_log();
	bool bSuccess = true;
	if (FileExists())
	{
		if (makeBackup)
		{
			// backup does a move, so no need to delete if it works
			bSuccess = backup_pref_file(mStateKeyName, shouldLog);
			if (!bSuccess)
				bSuccess = std::remove(mStateKeyName.c_str()) == 0 ? true : false;
		}
		else
			bSuccess = std::remove(mStateKeyName.c_str()) == 0 ? true : false;
		if (bSuccess)
			AJA_LOG_NOTICE(shouldLog, "db file '" << mStateKeyName << "' deleted");
		else
			AJA_LOG_WARN(shouldLog, "failed to delete db file '" << mStateKeyName << "'");
	}
	else
		AJA_LOG_WARN(shouldLog, "failed to delete db file, not found");
	return bSuccess;
}

bool AJAPersistence::StorageHealthCheck(int& errCode, std::string& errMessage)
{
	if (!FileExists())
	{
		errCode = -1;
		errMessage = "db file does not exist";
		return false;
	}

	AJAPersistenceDBImpl db(mStateKeyName);
	if (db.IsDBOpen())
	{
		errCode = 0;
		errMessage = "";
		return true;
	}

	errCode = db.OpenErrorCode();
	errMessage = sqlite3_errstr(errCode);
	return false;
}
