/**
	@file		persistence/persistence.cpp
	@copyright	Copyright (C) 2009-2018 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJAPersistence class.
**/

#include "persistence.h"
#include "sqlite3.h"
#include "stdlib.h"

#include <iostream>

#include "ajabase/common/common.h"
#include "ajabase/system/debug.h"
#include "ajabase/system/file_io.h"
#include "ajabase/system/systemtime.h"

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
#endif

#if defined(AJA_WINDOWS)
#include <Windows.h>
#include <io.h>
#pragma warning(disable:4996)
#ifndef F_OK
#define F_OK 0
#endif
#endif

#if defined(AJA_LINUX)
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifndef F_OK
#define F_OK 0
#endif
#endif

// Reduce the typing when using the logging macros
#define AJA_LOG_DEBUG(_expr_)    AJA_sDEBUG(AJA_DebugUnit_Persistence, _expr_)
#define AJA_LOG_INFO(_expr_)     AJA_sINFO(AJA_DebugUnit_Persistence, _expr_)
#define AJA_LOG_NOTICE(_expr_)   AJA_sNOTICE(AJA_DebugUnit_Persistence, _expr_)
#define AJA_LOG_WARN(_expr_)     AJA_sWARNING(AJA_DebugUnit_Persistence, _expr_)
#define AJA_LOG_ERROR(_expr_)    AJA_sERROR(AJA_DebugUnit_Persistence, _expr_)
//#define AJA_LOG_ALERT(_expr_)    AJA_sALERT(AJA_DebugUnit_Persistence, _expr_)
//#define AJA_LOG_EMERGENCY(_expr_) AJA_sEMERGENCY(AJA_DebugUnit_Persistence, _expr_)
//#define AJA_LOG_ASSERT(_expr_)   AJA_sASSERT(AJA_DebugUnit_Persistence, _expr_)

// Use deferent log levels so can better sort reads/writes
#define AJA_LOG_READ(_expr_)     AJA_LOG_INFO(_expr_)
#define AJA_LOG_WRITE(_expr_)    AJA_LOG_NOTICE(_expr_)

inline bool should_we_log()
{
    int32_t refCount = 0;
    AJADebug::GetClientReferenceCount(&refCount);
    return (refCount > 0);
}

// Encapsulate the sqlite3 object so automatically handled by constructor/destructor
class AJAPersistenceDBImplObject
{
public:
        AJAPersistenceDBImplObject(const std::string &pathToDB)
        : mDb(NULL), mPath(""), mOpenErrorCode(SQLITE_ERROR)
        {
            mPath = pathToDB;
            mOpenErrorCode = sqlite3_open_v2(mPath.c_str(), &mDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
            if (should_we_log())
            {
                if (mOpenErrorCode != SQLITE_OK)
                {
                    AJA_LOG_ERROR("sqlite> error code: " << mOpenErrorCode <<
                                  " with message: \"" << sqlite3_errmsg(mDb) << "\" when opening DB at: " << mPath);
                }
                else
                {
                    AJA_LOG_WRITE("sqlite> successfully opened handle to DB at: " << mPath);
                }
            }
        }

        virtual ~AJAPersistenceDBImplObject()
        {
            int rc = sqlite3_close(mDb);
            if (should_we_log())
            {
                if (rc != SQLITE_OK)
                {
                    AJA_LOG_ERROR("sqlite> error code: " << mOpenErrorCode <<
                                  " with message: \"" << sqlite3_errmsg(mDb) << "\" when closing DB at: " << mPath);
                }
                else
                {
                    AJA_LOG_WRITE("sqlite> successfully closed handle to DB at: " << mPath);
                }
            }
        }

        bool IsDBOpen()         { return (mOpenErrorCode == SQLITE_OK); }
        int OpenErrorCode()     { return mOpenErrorCode; }
        std::string PathToDB()  { return mPath; }
        sqlite3* GetHandle()    { return mDb; }

private:
        sqlite3 *mDb;
        std::string mPath;
        int mOpenErrorCode;
};

// Encapsulate the sqlite3_stmt object so automatically handled by constructor/destructor
const int gDefaultNumSqliteRetries = 10;
const int32_t gDefaultMicrosecondsBetweenRetries = 1500;

class AJAPersistenceDBImplStatement
{
public:
        AJAPersistenceDBImplStatement()
        : mStmt(NULL), mStmtString(""), mPrepareErrorCode(SQLITE_ERROR),
          mNumRetries(gDefaultNumSqliteRetries), mMicrosecondsBetweenRetries(gDefaultMicrosecondsBetweenRetries)
        {
        }

        AJAPersistenceDBImplStatement(AJAPersistenceDBImplObject &db, const std::string &stmt)
        : mStmt(NULL), mStmtString(""), mPrepareErrorCode(SQLITE_ERROR),
          mNumRetries(gDefaultNumSqliteRetries), mMicrosecondsBetweenRetries(gDefaultMicrosecondsBetweenRetries)
        {
            Prepare(db, stmt);
        }

        int Prepare(AJAPersistenceDBImplObject &db, const std::string &stmt)
        {
            bool shouldLog = should_we_log();

            mStmtString = stmt;
            for(int i=0;i<mNumRetries;i++)
            {
                mPrepareErrorCode = sqlite3_prepare_v3(db.GetHandle(), mStmtString.c_str(), -1, SQLITE_PREPARE_PERSISTENT, &mStmt, NULL);
                if (mPrepareErrorCode != SQLITE_OK)
                {                                      
                    std::ostringstream oss;
                    if (shouldLog)
                    {
                        oss << "sqlite> attempt: " << i+1 << " of " << mNumRetries << ", error code: " << mPrepareErrorCode <<
                               " with message: \"" << sqlite3_errmsg(db.GetHandle()) << "\" when preparing statement: " << mStmtString;
                    }

                    if (i == mNumRetries-1)
                    {
                        if (shouldLog)
                            AJA_LOG_ERROR(oss.str());
                    }
                    else
                    {
                        if (shouldLog)
                            AJA_LOG_WARN(oss.str());

                        AJATime::SleepInMicroseconds(mMicrosecondsBetweenRetries);
                    }
                }
                else
                {
                    if (shouldLog)
                        AJA_LOG_WRITE("sqlite> successfully prepared statement: " << mStmtString);

                    break;
                }
            }
            return mPrepareErrorCode;
        }

        virtual ~AJAPersistenceDBImplStatement()
        {
            sqlite3_finalize(mStmt);
        }

        int Reset()
        {
            int rc = sqlite3_reset(mStmt);
            return rc;
        }

        int BindText(int parameterNum, const std::string& value)
        {
            int rc = sqlite3_bind_text(mStmt, parameterNum, value.c_str(), -1, NULL);
            return rc;
        }

        int BindBlob(int parameterNum, int blobSizeInBytes, const void *value)
        {
            int rc = sqlite3_bind_blob(mStmt, parameterNum, value, blobSizeInBytes, SQLITE_TRANSIENT);
            return rc;
        }

        int Step()
        {
            bool shouldLog = should_we_log();

            int rc = SQLITE_OK;

            for(int i=0;i<mNumRetries;i++)
            {
                rc = sqlite3_step(mStmt);
                if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE)
                {
                    std::ostringstream oss;
                    if (shouldLog)
                    {
                        oss << "sqlite> attempt: " << i+1 << " of " << mNumRetries << ", error code: " << rc <<
                               " with message: \"" << sqlite3_errstr(rc) << "\" when stepping statement: " << sqlite3_expanded_sql(mStmt);
                    }

                    if (i == (mNumRetries)-1)
                    {
                        if (shouldLog)
                            AJA_LOG_ERROR(oss.str());
                    }
                    else
                    {
                        if (shouldLog)
                            AJA_LOG_WARN(oss.str());

                        AJATime::SleepInMicroseconds(mMicrosecondsBetweenRetries);
                    }
                }
                else
                {
                    //if (shouldLog)
                    //AJA_LOG_WRITE("sqlite> successfully stepped statement: " << sqlite3_expanded_sql(mStmt));
                    break;
                }
            }
            return rc;
        }

        std::string ColumnText(int col)
        {
            return std::string((const char*)sqlite3_column_text(mStmt, col));
        }

        bool ColumnBlob(int col, void* output, int& blobSize)
        {
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

        sqlite3_stmt* GetHandle()
        {
            return mStmt;
        }

private:
        sqlite3_stmt *mStmt;
        std::string mStmtString;
        int mPrepareErrorCode;
        int mNumRetries;
        int32_t mMicrosecondsBetweenRetries;
};

class AJAPersistenceDBImpl
{
public:
        AJAPersistenceDBImpl(const std::string &pathToDB)
        : mDb(pathToDB)
        {
            if (mDb.IsDBOpen())
            {
                // generic table statements
                AJAPersistenceDBImplStatement createTableStmt(mDb, "CREATE TABLE IF NOT EXISTS persistence(id INTEGER, name CHAR(255), value CHAR(64), dev_name CHAR(64), dev_num CHAR(64), PRIMARY KEY(id))");
                AJAPersistenceDBImplStatement blobCreateTableStmt(mDb, "CREATE TABLE IF NOT EXISTS persistenceBlobs(id INTEGER, name CHAR(255), value BLOB, dev_name CHAR(64), dev_num CHAR(64), PRIMARY KEY(id))");

                mClearTableStmt.Prepare(mDb, "DELETE FROM persistence");
                mBlobClearTableStmt.Prepare(mDb, "DELETE FROM persistenceBlobs");
                mVacuumDBStmt.Prepare(mDb, "VACUUM");

                // The tables must exist before the other prepared statements can be made
                mTableCreateErrorCode = createTableStmt.Step();
                mTableCreateBlobErrorCode = blobCreateTableStmt.Step();

                // normal table statements
                mGetValueSpecificStmt.Prepare(mDb, "SELECT value FROM persistence WHERE name=?1 AND dev_name=?2 AND dev_num=?3");
                mGetValueLessSpecificStmt.Prepare(mDb, "SELECT value FROM persistence WHERE name=?1 AND dev_name=?2");
                mUpdateSetStmt.Prepare(mDb, "INSERT OR REPLACE INTO persistence (id, name, value, dev_name, dev_num) SELECT id, ?1, ?2, ?3, ?4 "
                                            "FROM ( SELECT NULL ) LEFT JOIN ( SELECT * FROM persistence WHERE name=?5 AND dev_name=?6 AND dev_num=?7)");


                // blob table statements
                mBlobGetValueSpecificStmt.Prepare(mDb, "SELECT value FROM persistenceBlobs WHERE name=?1 AND dev_name=?2 AND dev_num=?3");
                mBlobGetValueLessSpecificStmt.Prepare(mDb, "SELECT value FROM persistenceBlobs WHERE name=?1 AND dev_name=?2");
                mBlobUpdateSetStmt.Prepare(mDb, "INSERT OR REPLACE INTO persistenceBlobs (id, name, value, dev_name, dev_num) SELECT id, ?1, ?2, ?3, ?4 "
                                                "FROM ( SELECT NULL ) LEFT JOIN ( SELECT * FROM persistenceBlobs WHERE name=?5 AND dev_name=?6 AND dev_num=?7)");

                // multiple return statements
                mGetAllValuesSpecificStmt.Prepare(mDb, "SELECT name, value FROM persistence WHERE name LIKE ?1 AND dev_name=?2 AND dev_num=?3");
                mGetAllValuesLessSpecificStmt.Prepare(mDb, "SELECT name, value FROM persistence WHERE name LIKE ?1 AND dev_name=?2");
            }
            else
            {
                if (should_we_log())
                    AJA_LOG_ERROR("sqlite> could not prepare statements DB not opened");
            }
        }

        virtual ~AJAPersistenceDBImpl()
        {
        }

        bool ClearTables()
        {
            mClearTableStmt.Reset();
            mBlobClearTableStmt.Reset();
            mVacuumDBStmt.Reset();

            int rc = 0;
            rc = mClearTableStmt.Step();
            mBlobClearTableStmt.Step();
            mVacuumDBStmt.Step();

            return (rc == SQLITE_OK || rc == SQLITE_DONE);
        }

        bool ConvertStringToValueType(const std::string& inputString, AJAPersistenceType type, void *outputValue)
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

        bool ConvertValueTypeToString(void *inputValue, AJAPersistenceType type, std::string& outputString)
        {
            bool isGood = false;
            switch(type)
            {
                case AJAPersistenceTypeInt:
                {
                    outputString = aja::to_string(*((int*)inputValue));
                    isGood = true;
                }
                break;

                case AJAPersistenceTypeBool:
                {
                    outputString = (*((bool*)inputValue) == true) ? "1" : "0";
                    isGood = true;
                }
                break;

                case AJAPersistenceTypeDouble:
                {
                    outputString = aja::to_string(*((double*)inputValue));
                    isGood = true;
                }
                break;

                case AJAPersistenceTypeString:
                {
                    outputString = *((std::string *)inputValue);
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
            if (mDb.IsDBOpen())
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

        bool SetValue(std::string keyQuery, void *value, AJAPersistenceType type, int blobSizeInBytes, std::string devName = "", std::string devNum = "")
        {
            bool isGood = false;
            if (mDb.IsDBOpen())
            {
                AJAPersistenceDBImplStatement *updateSetStmt;

                std::string strValue;
                bool goodToSet = false;
                if (type == AJAPersistenceTypeBlob)
                {
                    updateSetStmt = &mBlobUpdateSetStmt;
                    goodToSet     = true;
                }
                else
                {
                    updateSetStmt = &mUpdateSetStmt;
                    goodToSet     = ConvertValueTypeToString(value, type, strValue);
                }

                if (goodToSet)
                {
                    // Reset statement and bind parameters
                    updateSetStmt->Reset();

                    if (type == AJAPersistenceTypeBlob)
                    {
                        updateSetStmt->BindText(1, keyQuery);
                        updateSetStmt->BindBlob(2, blobSizeInBytes, value);
                        updateSetStmt->BindText(3, devName);
                        updateSetStmt->BindText(4, devNum);
                        updateSetStmt->BindText(5, keyQuery);
                        updateSetStmt->BindText(6, devName);
                        updateSetStmt->BindText(7, devNum);
                    }
                    else
                    {
                        updateSetStmt->BindText(1, keyQuery);
                        updateSetStmt->BindText(2, strValue);
                        updateSetStmt->BindText(3, devName);
                        updateSetStmt->BindText(4, devNum);
                        updateSetStmt->BindText(5, keyQuery);
                        updateSetStmt->BindText(6, devName);
                        updateSetStmt->BindText(7, devNum);
                    }

                    if (updateSetStmt->Step() == SQLITE_DONE)
                    {
                        // update was good
                        isGood = true;
                    }
                }
            }

            return isGood;
        }

        bool GetAllMatchingValues(std::string keyQuery, std::vector<std::string>& keys, std::vector<std::string>& values,
                                  std::string devName = "", std::string devNum = "")
        {
            bool isGood = false;
            if (mDb.IsDBOpen())
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

private:
      AJAPersistenceDBImplObject    mDb;
      int                           mTableCreateErrorCode;
      int                           mTableCreateBlobErrorCode;

      // generic statements
      AJAPersistenceDBImplStatement mClearTableStmt;
      AJAPersistenceDBImplStatement mBlobClearTableStmt;
      AJAPersistenceDBImplStatement mVacuumDBStmt;

      // normal statements
      AJAPersistenceDBImplStatement mGetValueSpecificStmt;
      AJAPersistenceDBImplStatement mGetValueLessSpecificStmt;
      AJAPersistenceDBImplStatement mUpdateSetStmt;

      // blob statements
      AJAPersistenceDBImplStatement mBlobGetValueSpecificStmt;
      AJAPersistenceDBImplStatement mBlobGetValueLessSpecificStmt;
      AJAPersistenceDBImplStatement mBlobUpdateSetStmt;

      // multiple return statements
      AJAPersistenceDBImplStatement mGetAllValuesSpecificStmt;
      AJAPersistenceDBImplStatement mGetAllValuesLessSpecificStmt;
};

//MARK: Start of Class

AJAPersistence::AJAPersistence()
    : mDBImpl(NULL)
{
    SetParams("null_device");
}

AJAPersistence::AJAPersistence(const std::string& appID, const std::string& deviceType, const std::string& deviceNumber, bool bSharePrefFile)
    : mDBImpl(NULL)
{
    SetParams(appID, deviceType, deviceNumber, bSharePrefFile);
}

AJAPersistence::~AJAPersistence()
{
    if (mDBImpl)
    {
        delete mDBImpl;
        mDBImpl = NULL;
    }
}

void AJAPersistence::SetParams(const std::string& appID, const std::string& deviceType, const std::string& deviceNumber, bool bSharePrefFile)
{
    std::string lastStateKeyName = mstateKeyName;

    mappId          = appID;
    mboardId        = deviceType;
    mserialNumber	= deviceNumber;
    mSharedPrefFile = bSharePrefFile;

    if (mSharedPrefFile)
    {
        mSysInfo.GetValue(AJA_SystemInfoTag_Path_PersistenceStoreSystem, mstateKeyName);
    }
    else
    {
        mSysInfo.GetValue(AJA_SystemInfoTag_Path_PersistenceStoreUser, mstateKeyName);
    }

    mstateKeyName += appID;

    bool shouldLog = should_we_log();
    if (mDBImpl && lastStateKeyName != mstateKeyName)
    {
        if (shouldLog)
            AJA_LOG_INFO("deleting existing db instance called from SetParams");

        delete mDBImpl;
        mDBImpl = NULL;
    }

    if (mDBImpl == NULL)
    {
        if (shouldLog)
            AJA_LOG_INFO("creating db instance called from SetParams");

        mDBImpl = new AJAPersistenceDBImpl(mstateKeyName);
    }
}

void AJAPersistence::GetParams(std::string& appID, std::string& deviceType, std::string& deviceNumber, bool& bSharePrefFile)
{
    appID = mappId;
    deviceType = mboardId;
    deviceNumber = mserialNumber;
    bSharePrefFile = mSharedPrefFile;
}

bool AJAPersistence::SetValue(const std::string& key, void *value, AJAPersistenceType type, int blobSize)
{
    if (should_we_log())
        AJA_LOG_WRITE("writing value of type: " << type << " , with key: " << key);

    bool isGood = false;
    if (mDBImpl)
    {
        isGood = mDBImpl->SetValue(key, value, type, blobSize, mboardId, mserialNumber);
    }
    return isGood;
}

bool AJAPersistence::GetValue(const std::string& key, void *value, AJAPersistenceType type, int blobSize)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;

    if (should_we_log())
        AJA_LOG_READ("reading value of type: " << type << " , with key: " << key);

    bool isGood = false;
    if (mDBImpl)
    {
        isGood = mDBImpl->GetValue(key, value, type, blobSize, mboardId, mserialNumber);
    }
    return isGood;
}

bool AJAPersistence::GetValuesString(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<std::string>& values)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;

    if (should_we_log())
        AJA_LOG_READ("reading string values with query key: " << keyQuery);

    bool isGood = false;
    if (mDBImpl)
    {
        isGood = mDBImpl->GetAllMatchingValues(keyQuery, keys, values, mboardId, mserialNumber);
    }
    return isGood;
}

bool AJAPersistence::GetValuesInt(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<int>& values)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;

    if (should_we_log())
        AJA_LOG_READ("reading int values with query key: " << keyQuery);
	
	std::vector<std::string> tmpValues;
    if (GetValuesString(keyQuery, keys, tmpValues))
	{
		for (int i = 0; i < (int)keys.size(); i++)
		{
			values.push_back(atoi(tmpValues.at(i).c_str()));
		}

		return true;
	}
	return false;
}

bool AJAPersistence::GetValuesBool(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<bool>& values)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;

    if (should_we_log())
        AJA_LOG_READ("reading bool values with query key: " << keyQuery);

	std::vector<std::string> tmpValues;
    if (GetValuesString(keyQuery, keys, tmpValues))
	{
		for (int i = 0; i < (int)keys.size(); i++)
		{
			values.push_back((atoi(tmpValues.at(i).c_str()) == 1) ? true : false);
		}

		return true;
	}
	return false;
}

bool AJAPersistence::GetValuesDouble(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<double>& values)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;

    if (should_we_log())
        AJA_LOG_READ("reading double values with query key: " << keyQuery);

	std::vector<std::string> tmpValues;
    if (GetValuesString(keyQuery, keys, tmpValues))
	{
		for (int i = 0; i < (int)keys.size(); i++)
		{
			values.push_back(atof(tmpValues.at(i).c_str()));
		}

		return true;
	}
	return false;
}

bool AJAPersistence::FileExists()
{
    return AJAFileIO::FileExists(mstateKeyName.c_str());
}

bool AJAPersistence::ClearPrefFile()
{
    bool shouldLog = should_we_log();
    bool bSuccess = true;
    if (FileExists())
    {
        if (mDBImpl)
        {
            if (shouldLog)
                AJA_LOG_INFO("clearing existing tables in db instance called from ClearPrefFile");

            bSuccess = mDBImpl->ClearTables();
        }
        else
        {
            if (shouldLog)
                AJA_LOG_NOTICE("could not clear existing tables in db, instance not found, called from ClearPrefFile");

            bSuccess = false;
        }
    }
    else
    {
        if (shouldLog)
            AJA_LOG_NOTICE("could not clear existing tables in db, file not found, called from ClearPrefFile");
    }
    return bSuccess;
}

// delete pref file
bool AJAPersistence::DeletePrefFile()
{
    bool shouldLog = should_we_log();
	bool bSuccess = true;
	if (FileExists())
	{
        if (mDBImpl)
        {
            if (shouldLog)
                AJA_LOG_INFO("deleting existing db instance called from DeletePrefFile");

            delete mDBImpl;
            mDBImpl = NULL;
        }

		int err = remove(mstateKeyName.c_str());
		bSuccess = err != 0;

        if (shouldLog)
            AJA_LOG_INFO("creating db instance called from DeletePrefFile");

        mDBImpl = new AJAPersistenceDBImpl(mstateKeyName);
	}
    else
    {
        if (shouldLog)
            AJA_LOG_NOTICE("could not delete existing db, file not found, called from DeletePrefFile");
    }
	return bSuccess;
}