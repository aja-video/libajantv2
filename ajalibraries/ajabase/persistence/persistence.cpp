/**
	@file		persistence/persistence.cpp
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJAPersistence class.
**/

#include "persistence.h"
#include "sqlite3.h"
#include "stdlib.h"
//#include "ajabase/system/debug.h"

#include "ajabase/system/file_io.h"

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

//MARK: hidden helpers

std::string makeCreateTableString(const std::string& tableName,bool blobTable = false)
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

bool PersistenceSetValue(std::string keyRoot, std::string key, void *value, AJAPersistenceType type, std::string devName = "", std::string devNum = "")
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

bool PersistenceSetValueBlob(std::string keyRoot, std::string key, void *value, int blobBytes, std::string devName = "", std::string devNum = "")
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

bool PersistenceGetValue(std::string keyRoot, std::string key, void *value, AJAPersistenceType type, std::string devName = "", std::string devNum = "")
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

	// Try reading value from more harware specific to general

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

// This version with return multiple rows that match the 'key_query', in the query use '_' for a single character match
// use '%' for multi character match. See http://www.sqlite.org/lang_expr.html for tips how the LIKE operator works in SQL expressions
bool PresistenceGetValues(std::string keyRoot, std::string key_query, std::vector<std::string>& keys, std::vector<std::string>& values,
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

bool PersistenceGetValueBlob(std::string keyRoot, std::string key, void *value, int blobBytes, std::string devName = "", std::string devNum = "")
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

//MARK: Start of Class

AJAPersistence::AJAPersistence()		
{
    SetParams("null_device");
}

AJAPersistence::AJAPersistence(const std::string& appID, const std::string& deviceType, const std::string& deviceNumber, bool bSharePrefFile)
{
    SetParams(appID, deviceType, deviceNumber, bSharePrefFile);
}

AJAPersistence::~AJAPersistence()
{
	
}

void AJAPersistence::SetParams(const std::string& appID, const std::string& deviceType, const std::string& deviceNumber, bool bSharePrefFile)
{
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
	if(type == AJAPersistenceTypeBlob)
		return ::PersistenceSetValueBlob(mstateKeyName, key, value, blobSize, mboardId, mserialNumber);
	else
		return ::PersistenceSetValue(mstateKeyName, key, value, type, mboardId, mserialNumber);
}

bool AJAPersistence::GetValue(const std::string& key, void *value, AJAPersistenceType type, int blobSize)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;

	if(type == AJAPersistenceTypeBlob)
		return ::PersistenceGetValueBlob(mstateKeyName, key, value, blobSize, mboardId, mserialNumber);
	else
		return ::PersistenceGetValue(mstateKeyName, key, value, type, mboardId, mserialNumber);
}

bool AJAPersistence::GetValuesString(const std::string& key_query, std::vector<std::string>& keys, std::vector<std::string>& values)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;

	return ::PresistenceGetValues(mstateKeyName, key_query, keys, values, mboardId, mserialNumber);
}

bool AJAPersistence::GetValuesInt(const std::string& key_query, std::vector<std::string>& keys, std::vector<int>& values)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;
	
	std::vector<std::string> tmpValues;
	if (GetValuesString(key_query, keys, tmpValues))
	{
		for (int i = 0; i < (int)keys.size(); i++)
		{
			values.push_back(atoi(tmpValues.at(i).c_str()));
		}

		return true;
	}
	return false;
}

bool AJAPersistence::GetValuesBool(const std::string& key_query, std::vector<std::string>& keys, std::vector<bool>& values)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;

	std::vector<std::string> tmpValues;
	if (GetValuesString(key_query, keys, tmpValues))
	{
		for (int i = 0; i < (int)keys.size(); i++)
		{
			values.push_back((atoi(tmpValues.at(i).c_str()) == 1) ? true : false);
		}

		return true;
	}
	return false;
}

bool AJAPersistence::GetValuesDouble(const std::string& key_query, std::vector<std::string>& keys, std::vector<double>& values)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;

	std::vector<std::string> tmpValues;
	if (GetValuesString(key_query, keys, tmpValues))
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
		int err = remove(mstateKeyName.c_str());
		bSuccess = err != 0;
	}
	return bSuccess;
}
