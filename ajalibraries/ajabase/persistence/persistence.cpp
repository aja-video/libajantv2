/**
	@file		persistence/persistence.cpp
	@copyright	Copyright (C) 2009-2018 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJAPersistence class.
**/

#include "persistence.h"
#include "sqlite3.h"
#include "stdlib.h"
//#include "ajabase/system/debug.h"
#include <iostream>

#include "ajabase/common/common.h"
#include "ajabase/system/file_io.h"

#define AJA_FEATURE_FLAG_USE_NEW_SQLITE_IMPL 1

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

// Use deferent log levels so can better sort reads/writes
#define AJA_LOG_READ(_expr_)     AJA_sINFO(AJA_DebugUnit_Persistence, _expr_)
#define AJA_LOG_WRITE(_expr_)    AJA_sNOTICE(AJA_DebugUnit_Persistence, _expr_)
#define AJA_LOG_ERROR(_expr_)    AJA_sERROR(AJA_DebugUnit_Persistence, _expr_)

// Encapsulate the sqlite3 object so automatically handled by constructor/destructor
class AJAPersistenceDBImplObject
{
public:
        AJAPersistenceDBImplObject(const std::string &pathToDB)
        : mDb(NULL), mPath(""), mOpenErrorCode(SQLITE_ERROR)
        {
            mPath = pathToDB;
            mOpenErrorCode = sqlite3_open_v2(mPath.c_str(), &mDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
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

        virtual ~AJAPersistenceDBImplObject()
        {
            int rc = sqlite3_close(mDb);
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
class AJAPersistenceDBImplStatement
{
public:
        AJAPersistenceDBImplStatement()
        : mStmt(NULL), mStmtString(""), mPrepareErrorCode(SQLITE_ERROR)
        {
        }

        AJAPersistenceDBImplStatement(AJAPersistenceDBImplObject &db, const std::string &stmt)
        : mStmt(NULL), mStmtString(""), mPrepareErrorCode(SQLITE_ERROR)
        {
            Prepare(db, stmt);
        }

        int Prepare(AJAPersistenceDBImplObject &db, const std::string &stmt)
        {
            mStmtString = stmt;
            mPrepareErrorCode = sqlite3_prepare_v2(db.GetHandle(), mStmtString.c_str(), -1, &mStmt, NULL);

            if (mPrepareErrorCode != SQLITE_OK)
            {
                AJA_LOG_ERROR("sqlite> error code: " << mPrepareErrorCode <<
                              " with message: \"" << sqlite3_errmsg(db.GetHandle()) << "\" when preparing statement: " << mStmtString);
            }
            else
            {
                AJA_LOG_WRITE("sqlite> successfully prepared statement: " << mStmtString);
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
            int rc = sqlite3_step(mStmt);
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
};

class AJAPersistenceDBImpl
{
public:
        AJAPersistenceDBImpl(const std::string &pathToDB)
        : mDb(pathToDB)
        {
            if (mDb.IsDBOpen())
            {
                mCreateTableStmt.Prepare(mDb, "CREATE TABLE IF NOT EXISTS persistence(id INTEGER, name CHAR(255), value CHAR(64), dev_name CHAR(64), dev_num CHAR(64), PRIMARY KEY(id))");
                mBlobCreateTableStmt.Prepare(mDb, "CREATE TABLE IF NOT EXISTS persistenceBlobs(id INTEGER, name CHAR(255), value BLOB, dev_name CHAR(64), dev_num CHAR(64), PRIMARY KEY(id))");

                // The tables must exist before the other prepared statements can be made
                mTableCreateErrorCode = mCreateTableStmt.Step();
                mBlobTableCreateErrorCode = mBlobCreateTableStmt.Step();

                // normal table statements
                mGetValueSpecificStmt.Prepare(mDb, "SELECT value FROM persistence WHERE name=?1 AND dev_name=?2 AND dev_num=?3");
                mGetValueLessSpecificStmt.Prepare(mDb, "SELECT value FROM persistence WHERE name=?1 AND dev_name=?2");
                mUpdateValueStmt.Prepare(mDb, "UPDATE persistence SET value=?1 WHERE name=?2 AND dev_name=?3 AND dev_num=?4");
                mSetValueStmt.Prepare(mDb, "INSERT INTO persistence (name,value,dev_name,dev_num) values(?1,?2,?3,?4)");

                // blob table statements
                mBlobGetValueSpecificStmt.Prepare(mDb, "SELECT value FROM persistenceBlobs WHERE name=?1 AND dev_name=?2 AND dev_num=?3");
                mBlobGetValueLessSpecificStmt.Prepare(mDb, "SELECT value FROM persistenceBlobs WHERE name=?1 AND dev_name=?2");
                mBlobUpdateValueStmt.Prepare(mDb, "UPDATE persistenceBlobs SET value=?1 WHERE name=?2 AND dev_name=?3 AND dev_num=?4");
                mBlobSetValueStmt.Prepare(mDb, "INSERT INTO persistenceBlobs (name,value,dev_name,dev_num) values(?1,?2,?3,?4)");

                // multiple return statements
                mGetAllValuesSpecificStmt.Prepare(mDb, "SELECT name, value FROM persistence WHERE name LIKE ?1 AND dev_name=?2 AND dev_num=?3");
                mGetAllValuesLessSpecificStmt.Prepare(mDb, "SELECT name, value FROM persistence WHERE name LIKE ?1 AND dev_name=?2");
                mGetAllValuesGenericStmt.Prepare(mDb, "SELECT name, value FROM persistence WHERE name LIKE ?1");
            }
            else
            {
                AJA_LOG_ERROR("sqlite> could not prepare statements DB not opened");
            }
        }

        virtual ~AJAPersistenceDBImpl()
        {
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
                AJAPersistenceDBImplStatement *getValueStmt;
                AJAPersistenceDBImplStatement *updateStmt;
                AJAPersistenceDBImplStatement *setStmt;

                std::string strValue;
                bool goodToSet = false;
                if (type == AJAPersistenceTypeBlob)
                {
                    getValueStmt = &mBlobGetValueSpecificStmt;
                    updateStmt   = &mBlobUpdateValueStmt;
                    setStmt      = &mBlobSetValueStmt;
                    goodToSet    = true;
                }
                else
                {
                    getValueStmt = &mGetValueSpecificStmt;
                    updateStmt   = &mUpdateValueStmt;
                    setStmt      = &mSetValueStmt;
                    goodToSet    = ConvertValueTypeToString(value, type, strValue);
                }

                if (goodToSet)
                {
                    // Reset statements and bind parameters
                    getValueStmt->Reset();
                    updateStmt->Reset();
                    setStmt->Reset();

                    getValueStmt->BindText(1, keyQuery);
                    getValueStmt->BindText(2, devName);
                    getValueStmt->BindText(3, devNum);

                    if (type == AJAPersistenceTypeBlob)
                    {
                        updateStmt->BindBlob(1, blobSizeInBytes, value);
                        updateStmt->BindText(2, keyQuery);
                        updateStmt->BindText(3, devName);
                        updateStmt->BindText(4, devNum);

                        setStmt->BindText(1, keyQuery);
                        setStmt->BindBlob(2, blobSizeInBytes, value);
                        setStmt->BindText(3, devName);
                        setStmt->BindText(4, devNum);
                    }
                    else
                    {
                        updateStmt->BindText(1, strValue);
                        updateStmt->BindText(2, keyQuery);
                        updateStmt->BindText(3, devName);
                        updateStmt->BindText(4, devNum);

                        setStmt->BindText(1, keyQuery);
                        setStmt->BindText(2, strValue);
                        setStmt->BindText(3, devName);
                        setStmt->BindText(4, devNum);
                    }

                    // see if already have a matching row that needs update
                    if (getValueStmt->Step() == SQLITE_ROW)
                    {
                        if (updateStmt->Step() == SQLITE_DONE)
                        {
                            // update was good
                            isGood = true;
                        }
                    }
                    else
                    {
                        // do an INSERT of new data
                        if (setStmt->Step() == SQLITE_DONE)
                        {
                            isGood = true;
                        }
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
                mGetAllValuesGenericStmt.Reset();

                mGetAllValuesSpecificStmt.BindText(1, keyQuery);
                mGetAllValuesSpecificStmt.BindText(2, devName);
                mGetAllValuesSpecificStmt.BindText(3, devNum);

                mGetAllValuesLessSpecificStmt.BindText(1, keyQuery);
                mGetAllValuesLessSpecificStmt.BindText(2, devName);

                mGetAllValuesGenericStmt.BindText(1, keyQuery);

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

                if (keys.empty())
                {
                    ret_code = mGetAllValuesGenericStmt.Step();
                    while(ret_code == SQLITE_ROW)
                    {
                        std::string name = mGetAllValuesGenericStmt.ColumnText(0);
                        std::string value = mGetAllValuesGenericStmt.ColumnText(1);
                        keys.push_back(name);
                        values.push_back(value);

                        ret_code = mGetAllValuesGenericStmt.Step();
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
      int                           mBlobTableCreateErrorCode;

      // normal statements
      AJAPersistenceDBImplStatement mCreateTableStmt;
      AJAPersistenceDBImplStatement mGetValueSpecificStmt;
      AJAPersistenceDBImplStatement mGetValueLessSpecificStmt;
      AJAPersistenceDBImplStatement mUpdateValueStmt;
      AJAPersistenceDBImplStatement mSetValueStmt;

      // blob statements
      AJAPersistenceDBImplStatement mBlobCreateTableStmt;
      AJAPersistenceDBImplStatement mBlobGetValueSpecificStmt;
      AJAPersistenceDBImplStatement mBlobGetValueLessSpecificStmt;
      AJAPersistenceDBImplStatement mBlobUpdateValueStmt;
      AJAPersistenceDBImplStatement mBlobSetValueStmt;

      // multiple return statements
      AJAPersistenceDBImplStatement mGetAllValuesSpecificStmt;
      AJAPersistenceDBImplStatement mGetAllValuesLessSpecificStmt;
      AJAPersistenceDBImplStatement mGetAllValuesGenericStmt;
};

#if !defined(AJA_FEATURE_FLAG_USE_NEW_SQLITE_IMPL)
//MARK: hidden helpers

static std::string makeCreateTableString(const std::string& tableName,bool blobTable = false)
{
	std::string stmt;
	if (blobTable)
	{
		stmt = std::string("CREATE TABLE IF NOT EXISTS ") + tableName + "(id INTEGER, name CHAR(255), value BLOB, dev_name CHAR(64), dev_num CHAR(64), PRIMARY KEY(id));";
	}
	else
	{
		stmt = std::string("CREATE TABLE IF NOT EXISTS ") + tableName + "(id INTEGER, name CHAR(255), value CHAR(64), dev_name CHAR(64), dev_num CHAR(64), PRIMARY KEY(id));";
	}

	return stmt;
}

static bool PersistenceSetValue(std::string keyRoot, std::string key, void *value, AJAPersistenceType type, std::string devName = "", std::string devNum = "")
{
	//int64_t start = AJATime::GetSystemMilliseconds();

	bool isGood = false;

	sqlite3 *db;	
	char *zErrMsg = 0;
	int rc;
	
	const char *tableName = "persistence";
	
	std::string stmt;
	
	rc = sqlite3_open(keyRoot.c_str(), &db);
	
	if(rc != SQLITE_OK)
	{
		sqlite3_close(db);
		isGood = false;
		return isGood;
	}
	
	char **azResult;
	int nRows;
	int nCols;
	
	// Make sure table exists
	stmt = makeCreateTableString(tableName,false);

	rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);
	sqlite3_free(zErrMsg);zErrMsg=0;
	sqlite3_free_table(azResult);
	
	if(rc != SQLITE_OK)
	{	
		sqlite3_close(db);
		isGood = false;
		return isGood;		
	}
	
	char *valueAsStr=NULL;
	if(type == AJAPersistenceTypeString)
	{	
		std::string *valuePtr = (std::string *)value;	
		valueAsStr = new char[valuePtr->length()+1];
	}
	else
	{
		valueAsStr = new char[64];
	}	

	switch(type)
	{
		case AJAPersistenceTypeInt:	
		{
			int *inVal = (int*)value;
			sprintf(valueAsStr,"%d",*inVal);
			isGood = true;
		}
			break;
			
		case AJAPersistenceTypeBool:
		{
			bool *inVal = (bool*)value;
			if(*inVal == true)				
				sprintf(valueAsStr,"1");
			else
				sprintf(valueAsStr,"0");			
			isGood = true;
		}
			break;
			
		case AJAPersistenceTypeDouble:
		{
			double *inVal = (double*)value;
			sprintf(valueAsStr,"%f",*inVal);
			isGood = true;
		}
			break;
			
		case AJAPersistenceTypeString:
		{
			std::string *valuePtr = (std::string *)value;	
			sprintf(valueAsStr,"%s",valuePtr->c_str());
			isGood = true;
		}
			break;
			
		default:
			isGood = false;
			break;
	}
	
	if(isGood == false)
	{
		sqlite3_close(db);
		if(valueAsStr)
			delete []valueAsStr;
		return isGood;
	}
	
	// Try to update a value first, if not found do an insert
	// try matching dev_name & dev_num
	std::string whereClause = std::string("name='") + key.c_str() + "' AND dev_name='" + devName.c_str() + "' AND dev_num='" + devNum.c_str() + "'";
	stmt = std::string("SELECT value FROM ") + tableName + " WHERE " + whereClause + ";";
	rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);

	if (rc == SQLITE_OK && nRows > 0)
	{
		// do an update
		sqlite3_free(zErrMsg);zErrMsg=0;
		sqlite3_free_table(azResult);
		stmt = std::string("UPDATE ") + tableName +" SET value='" + valueAsStr + "' WHERE " + whereClause + ";";
	}
	else
	{
		// insert a new record
		stmt = std::string("INSERT INTO ") + tableName + " (name,value,dev_name,dev_num) values('" + key.c_str() + "','" + valueAsStr + "','" + devName.c_str() + "','" + devNum.c_str() + "');";
	}
	rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);

	if(valueAsStr)
		delete []valueAsStr;
	
	if(zErrMsg)
	{
		isGood = false;
    }
	else if(rc != SQLITE_OK)
	{
		isGood = false;
    }
	else		
	{	
		//all good
		isGood = true;
	}
	
	sqlite3_free(zErrMsg);
	sqlite3_free_table(azResult);
	
	sqlite3_close(db);
	
	//AJA_REPORT(5151,AJA_DebugSeverity_Debug,"[SetValue](%s) took %d",key.c_str(),AJATime::GetSystemMilliseconds() - start);

	return isGood; 
}

static bool PersistenceSetValueBlob(std::string keyRoot, std::string key, void *value, int blobBytes, std::string devName = "", std::string devNum = "")
{
	//int64_t start = AJATime::GetSystemMilliseconds();

	bool isGood = false;

	sqlite3 *db;	
	char *zErrMsg = 0;
	int rc;
	
	const char *tableName = "persistenceBlobs";
	
	std::string	stmt;

	rc = sqlite3_open(keyRoot.c_str(), &db);
	
	if(rc != SQLITE_OK)
	{
		sqlite3_close(db);
		isGood = false;
		return isGood;
	}
		
	char **azResult;
	int nRows;
	int nCols;
	
	// Make sure table exists
	stmt = makeCreateTableString(tableName,true);

	rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);
	sqlite3_free_table(azResult);
	
	if(rc != SQLITE_OK)
	{	
		sqlite3_close(db);
		isGood = false;
		return isGood;		
	}

	sqlite3_stmt*	prepStmt;

	// Try to update a value first, if not found do an insert
	// try matching dev_name & dev_num
	std::string whereClause = std::string("name='") + key.c_str() + "' AND dev_name='" + devName.c_str() + "' AND dev_num='" + devNum.c_str() + "'";
	stmt = std::string("SELECT value FROM ") + tableName + " WHERE " + whereClause + ";";
	rc = sqlite3_prepare_v2(db,stmt.c_str(),(int)strlen(stmt.c_str()),&prepStmt,0);
	bool isRecordFound = false;
	if (rc == SQLITE_OK)
	{
		rc = sqlite3_step(prepStmt);
		if(rc == SQLITE_ROW)
		{	
			isRecordFound = true;
		}
	}
	sqlite3_finalize(prepStmt);

	if (isRecordFound == false)
	{
		// try matching dev_name
		whereClause = std::string("name='") + key.c_str() + "' AND dev_name='" + devName.c_str() + "'";
		stmt = std::string("SELECT value FROM ") + tableName + " WHERE " + whereClause + ";";
		rc = sqlite3_prepare_v2(db,stmt.c_str(),(int)strlen(stmt.c_str()),&prepStmt,0);
		if (rc == SQLITE_OK)
		{
			rc = sqlite3_step(prepStmt);
			if(rc == SQLITE_ROW)
			{	
				isRecordFound = true;
			}
		}
		sqlite3_finalize(prepStmt);
	}

	if (isRecordFound)
	{
		// do an update
		stmt = std::string("UPDATE ") + tableName + " SET value=? WHERE " + whereClause + ";";
	}
	else
	{
		// insert a new record
		stmt = std::string("INSERT INTO ") + tableName + " (name,value,dev_name,dev_num) values('" + key.c_str() + "',?,'" + devName.c_str() + "','" + devNum.c_str() + "');";
	}

	rc = sqlite3_prepare_v2(db,stmt.c_str(),(int)strlen(stmt.c_str()),&prepStmt,0);
	if(rc != SQLITE_OK)
	{	
		sqlite3_finalize(prepStmt);
		sqlite3_close(db);
		isGood = false;
		return isGood;		
	}

	rc = sqlite3_bind_blob(prepStmt,1,value,blobBytes,SQLITE_TRANSIENT);
	if(rc != SQLITE_OK)
	{
		sqlite3_finalize(prepStmt);
		sqlite3_close(db);
		isGood = false;
		return isGood;	
	}

	rc = sqlite3_step(prepStmt);
	sqlite3_finalize(prepStmt);
	if(rc != SQLITE_DONE)
	{
		sqlite3_close(db);
		isGood = false;
		return isGood;	
	}
	else
	{
		isGood = true;
	}

	sqlite3_close(db);
	
	//AJA_REPORT(5151,AJA_DebugSeverity_Debug,"[SetValueBlob](%s) took %d",key.c_str(),AJATime::GetSystemMilliseconds() - start);

	return isGood;
}

// Sleeze out and do these as Macros for now

#define areResultsGood(boolVar,messageVar,rcVar,rowsVar,resultsVar) \
	boolVar = true; \
	if(messageVar) \
	{ \
		sqlite3_free(messageVar);messageVar=0; \
		boolVar = false; \
	} \
	else if(rcVar != SQLITE_OK) \
	{ \
		boolVar = false; \
	} \
	else if(rowsVar < 1) \
	{ \
		boolVar = false; \
	} \
\
	if(boolVar == false) \
	{ \
		sqlite3_free_table(resultsVar); \
	}

#define areResultsGoodBlob(boolVar,rcVar,prepVar) \
	boolVar = true; \
	if(rcVar != SQLITE_OK) \
	{ \
		sqlite3_finalize(prepVar); \
		boolVar = false; \
	} \
	else \
	{ \
		rcVar = sqlite3_step(prepVar); \
		if(rcVar != SQLITE_ROW) \
		{ \
			boolVar = false; \
		} \
	}

static bool PersistenceGetValue(std::string keyRoot, std::string key, void *value, AJAPersistenceType type, std::string devName = "", std::string devNum = "")
{
	//int64_t start = AJATime::GetSystemMilliseconds();

	bool isGood = false;
	
	sqlite3 *db;	
	char *zErrMsg = 0;
	int rc;
	
	const char *tableName = "persistence";
	
	std::string stmt;
	
	rc = sqlite3_open(keyRoot.c_str(), &db);
	
	if(rc != SQLITE_OK)
	{
		sqlite3_close(db);
		isGood = false;
		return isGood;
	}
	
	char **azResult;
	int nRows;
	int nCols;
	
	// Make sure table exists
	stmt = makeCreateTableString(tableName,false);
	
	rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);
	sqlite3_free_table(azResult);
	sqlite3_free(zErrMsg);zErrMsg=0;
	
	if(rc != SQLITE_OK)
	{	
		sqlite3_close(db);
		isGood = false;
		return isGood;		
	}

    // Try reading value from more hardware specific to general

	// try matching dev_name & dev_num
	std::string whereClause = std::string("name='") + key.c_str() + "' AND dev_name='" + devName.c_str() + "' AND dev_num='" + devNum.c_str() +"'";
	stmt = std::string("SELECT value FROM ") + tableName + " WHERE " + whereClause + ";";
	rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);
	areResultsGood(isGood,zErrMsg,rc,nRows,azResult);

	if (isGood == false)
	{
		// try matching dev_name
		whereClause = std::string("name='") + key.c_str() + "' AND dev_name='" + devName.c_str() + "'";
		stmt = std::string("SELECT value FROM ") + tableName + " WHERE " + whereClause + ";";
		rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);
		areResultsGood(isGood,zErrMsg,rc,nRows,azResult);

		// SS 12/5/2017 - Removed for now. Experience indicates non-identical devices 
		// sharing pref settings has been a source of many weird issues.
		/* if (isGood == false)
		{
			// try matching only keyname
			whereClause = std::string("name='") + key.c_str() + "'";
			stmt = std::string("SELECT value FROM ") + tableName + " WHERE " + whereClause + ";";
			rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);
			areResultsGood(isGood,zErrMsg,rc,nRows,azResult);
		} */
	}

	char *valueAsStr=NULL;

	if (isGood)
	{	
		//get value
		if(nRows > 0)
		{
            int valueLen = 64;

			// skip row 0, that contains column names
			int i = 1;		

			if(type == AJAPersistenceTypeString)
			{	
				valueLen = (int)strlen(azResult[i])+1;
			}

			valueAsStr = new char[valueLen];

			strncpy(valueAsStr,azResult[i],valueLen);
		}
		else 
			isGood = false;

		sqlite3_free_table(azResult);		
	}
	
	if(!isGood)
	{
		sqlite3_close(db);
		if(valueAsStr)
			delete []valueAsStr;

		return isGood;
	}
	
	switch(type)
	{
		case AJAPersistenceTypeInt:	
		{
			int *outVal = (int*)value;
			*outVal = atoi(valueAsStr);
			isGood = true;
		}
			break;
			
		case AJAPersistenceTypeBool:
		{
			bool *outVal = (bool*)value;
			int inVal = atoi(valueAsStr);
			if(inVal == 1)
				*outVal = true;
			else 
				*outVal = false;			
			isGood = true;
		}
			break;
			
		case AJAPersistenceTypeDouble:
		{
			double *outVal = (double*)value;
			*outVal = atof(valueAsStr);
			isGood = true;
		}
			break;
			
		case AJAPersistenceTypeString:
		{
			std::string *valuePtr = (std::string *)value;
			valuePtr->erase();
			valuePtr->append(valueAsStr);
			isGood = true;
		}
			break;
			
		default:
			isGood = false;
			break;
	}
	
	if(valueAsStr)
		delete []valueAsStr;

	sqlite3_close(db);	
	
	//AJA_REPORT(5151,AJA_DebugSeverity_Debug,"[GetValue](%s) took %d",key.c_str(),AJATime::GetSystemMilliseconds() - start);

	return isGood;
}

// This version will return multiple rows that match the 'key_query', in the query use '_' for a single character match
// use '%' for multi character match. See http://www.sqlite.org/lang_expr.html for tips how the LIKE operator works in SQL expressions
static bool PresistenceGetValues(std::string keyRoot, std::string key_query, std::vector<std::string>& keys, std::vector<std::string>& values,
								std::string devName = "", std::string devNum = "")
{
    bool isGood = false;

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    const char *tableName = "persistence";

    std::string stmt;

    rc = sqlite3_open(keyRoot.c_str(), &db);

    if(rc != SQLITE_OK)
    {
        sqlite3_close(db);
        isGood = false;
        return isGood;
    }

    char **azResult;
    int nRows;
    int nCols;

    // Make sure table exists
    stmt = makeCreateTableString(tableName,false);

    rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);
    sqlite3_free_table(azResult);
    sqlite3_free(zErrMsg);zErrMsg=0;

    if(rc != SQLITE_OK)
    {
        sqlite3_close(db);
        isGood = false;
        return isGood;
    }

    // Try reading value from more harware specific to general

	std::string select = std::string("SELECT name, value FROM ") + tableName;
	std::string whereClause3 = std::string(" WHERE name LIKE '") + key_query.c_str() + "'";
	std::string whereClause2 = whereClause3 + " AND dev_name='" + devName.c_str() + "'";
	std::string whereClause1 = whereClause2 + " AND dev_num = '" + devNum.c_str() +"'";

    // try matching dev_name & dev_num
    stmt = select + whereClause1 + ";";
    rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);
    areResultsGood(isGood,zErrMsg,rc,nRows,azResult);

    if (isGood == false)
    {
        // try matching dev_name
		stmt = select + whereClause2 + ";";
        rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);
        areResultsGood(isGood,zErrMsg,rc,nRows,azResult);

        if (isGood == false)
        {
            // try matching only keyname
			stmt = select + whereClause3 + ";";
            rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);
            areResultsGood(isGood,zErrMsg,rc,nRows,azResult);
        }
    }

    if (isGood)
    {
        //get value
        if(nRows > 0)
        {
            char colName[255];
            char value[64];
            int valueLen = 64;

            // skip row 0, contains col 1 name
			// skip row 1, contains col 2 name
            for(int r=nCols; r < ((nRows+1)*nCols); r+=nCols)
            {
				int colLen = (int)strlen(azResult[r]) + 1;
				strncpy(colName, azResult[r], colLen);

                valueLen = (int)strlen(azResult[r+1])+1;
                strncpy(value,azResult[r+1],valueLen);

				isGood = true;
				keys.push_back(colName);
				values.push_back(std::string(value));
            }
        }
        else
            isGood = false;

        sqlite3_free_table(azResult);
    }

    if(!isGood)
    {
        sqlite3_close(db);
        return isGood;
    }

    sqlite3_close(db);

    //AJA_REPORT(5151,AJA_DebugSeverity_Debug,"[GetValues](%s) took %d",key.c_str(),AJATime::GetSystemMilliseconds() - start);

    return isGood;
}

static bool PersistenceGetValueBlob(std::string keyRoot, std::string key, void *value, int blobBytes, std::string devName = "", std::string devNum = "")
{
	//int64_t start = AJATime::GetSystemMilliseconds();

	bool isGood = false;

	sqlite3 *db;	
	char *zErrMsg = 0;
	int rc;
	
	const char *tableName = "persistenceBlobs";
	
	std::string	stmt;

	rc = sqlite3_open(keyRoot.c_str(), &db);
	
	if(rc != SQLITE_OK)
	{
		sqlite3_close(db);
		isGood = false;
		return isGood;
	}
	
	char **azResult;
	int nRows;
	int nCols;
	
	// Make sure table exists
	stmt = makeCreateTableString(tableName,true);

	rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);
	sqlite3_free_table(azResult);
	
	if(rc != SQLITE_OK)
	{	
		sqlite3_close(db);
		isGood = false;
		return isGood;		
	}

	// Try reading value from more harware specific to general
	sqlite3_stmt*	prepStmt;

	// try matching dev_name & dev_num
	std::string whereClause = std::string("name='") + key.c_str() + "' AND dev_name='" + devName.c_str() + "' AND dev_num='" + devNum.c_str() +"'";
	stmt = std::string("SELECT value FROM ") + tableName + " WHERE " + whereClause + ";";
	rc = sqlite3_prepare_v2(db,stmt.c_str(),(int)strlen(stmt.c_str()),&prepStmt,0);
	areResultsGoodBlob(isGood,rc,prepStmt);

	if (isGood == false)
	{
		// try matching dev_name
		whereClause = std::string("name='") + key.c_str() + "' AND dev_name='" + devName.c_str() + "'";
		stmt = std::string("SELECT value FROM ") + tableName + " WHERE " + whereClause + ";";
		rc = sqlite3_prepare_v2(db,stmt.c_str(),(int)strlen(stmt.c_str()),&prepStmt,0);
		areResultsGoodBlob(isGood,rc,prepStmt);

		if (isGood == false)
		{
			// try matching only keyname
			whereClause = std::string("name='") + key.c_str() + "'";
			stmt = std::string("SELECT value FROM ") + tableName + " WHERE " + whereClause + ";";
			rc = sqlite3_prepare_v2(db,stmt.c_str(),(int)strlen(stmt.c_str()),&prepStmt,0);
			areResultsGoodBlob(isGood,rc,prepStmt);
		}
	}

	if(isGood)
	{
		const int colWithData = 0;
		int newBlobSize = sqlite3_column_bytes(prepStmt,colWithData); 

		if(newBlobSize > 0 && newBlobSize <= blobBytes)
		{
			memcpy(value,sqlite3_column_blob(prepStmt,colWithData),newBlobSize);
		}
		else
		{
			isGood = false;	
		}
		sqlite3_finalize(prepStmt);
	}

	sqlite3_close(db);

	//AJA_REPORT(5151,AJA_DebugSeverity_Debug,"[GetValueBlob](%s) took %d",key.c_str(),AJATime::GetSystemMilliseconds() - start);
	
	return isGood;
}
#endif // !defined(AJA_FEATURE_FLAG_USE_NEW_SQLITE_IMPL)

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
#if defined(AJA_FEATURE_FLAG_USE_NEW_SQLITE_IMPL)
    if (mDBImpl)
    {
        delete mDBImpl;
        mDBImpl = NULL;
    }
#endif
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

#if defined(AJA_FEATURE_FLAG_USE_NEW_SQLITE_IMPL)
    if (mDBImpl && lastStateKeyName != mstateKeyName)
    {
        AJA_sINFO(AJA_DebugUnit_Persistence, "deleting existing db instance in SetParams");
        delete mDBImpl;
        mDBImpl = NULL;
    }

    if (mDBImpl == NULL)
    {
        AJA_sINFO(AJA_DebugUnit_Persistence, "creating db instance in SetParams");
        mDBImpl = new AJAPersistenceDBImpl(mstateKeyName);
    }
#endif
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
    AJA_LOG_WRITE("writing value of type: " << type << " , with key: " << key);

#if defined(AJA_FEATURE_FLAG_USE_NEW_SQLITE_IMPL)
    bool isGood = false;
    if (mDBImpl)
    {
        isGood = mDBImpl->SetValue(key, value, type, blobSize, mboardId, mserialNumber);
    }
    return isGood;
#else
    if(type == AJAPersistenceTypeBlob)
    {
        return ::PersistenceSetValueBlob(mstateKeyName, key, value, blobSize, mboardId, mserialNumber);
    }
    else
    {
        return ::PersistenceSetValue(mstateKeyName, key, value, type, mboardId, mserialNumber);
    }
#endif
}

bool AJAPersistence::GetValue(const std::string& key, void *value, AJAPersistenceType type, int blobSize)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;

    AJA_LOG_READ("reading value of type: " << type << " , with key: " << key);

#if defined(AJA_FEATURE_FLAG_USE_NEW_SQLITE_IMPL)
    bool isGood = false;
    if (mDBImpl)
    {
        isGood = mDBImpl->GetValue(key, value, type, blobSize, mboardId, mserialNumber);
    }
    return isGood;
#else
    if(type == AJAPersistenceTypeBlob)
        return ::PersistenceGetValueBlob(mstateKeyName, key, value, blobSize, mboardId, mserialNumber);
    else
        return ::PersistenceGetValue(mstateKeyName, key, value, type, mboardId, mserialNumber);
#endif
}

bool AJAPersistence::GetValuesString(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<std::string>& values)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;

    AJA_LOG_READ("reading string values with query key: " << keyQuery);

#if defined(AJA_FEATURE_FLAG_USE_NEW_SQLITE_IMPL)
    bool isGood = false;
    if (mDBImpl)
    {
        isGood = mDBImpl->GetAllMatchingValues(keyQuery, keys, values, mboardId, mserialNumber);
    }
    return isGood;
#else
    return ::PresistenceGetValues(mstateKeyName, keyQuery, keys, values, mboardId, mserialNumber);
#endif
}

bool AJAPersistence::GetValuesInt(const std::string& keyQuery, std::vector<std::string>& keys, std::vector<int>& values)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;

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

// delete pref file
bool AJAPersistence::DeletePrefFile()
{
	bool bSuccess = true;
	if (FileExists())
	{
#if defined(AJA_FEATURE_FLAG_USE_NEW_SQLITE_IMPL)
        if (mDBImpl)
        {
            AJA_sINFO(AJA_DebugUnit_Persistence, "deleting existing db instance in DeletePrefFile");
            delete mDBImpl;
            mDBImpl = NULL;
        }
#endif

		int err = remove(mstateKeyName.c_str());
		bSuccess = err != 0;

#if defined(AJA_FEATURE_FLAG_USE_NEW_SQLITE_IMPL)
        AJA_sINFO(AJA_DebugUnit_Persistence, "creating db instance in DeletePrefFile");
        mDBImpl = new AJAPersistenceDBImpl(mstateKeyName);
#endif
	}
	return bSuccess;
}
