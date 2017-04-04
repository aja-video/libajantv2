/**
	@file		persistence/persistence.cpp
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJAPersistence class.
**/

#include "persistence.h"
#include "sqlite3.h"
#include "stdlib.h"
//#include "ajabase/system/debug.h"
//#include "ajabase/system/systemtime.h"

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
// need to link with Shlwapi.lib
#include <Windows.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#include <io.h>
// #include <shfolder.h>
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

// SDK specific includes
#if defined(USE_WITH_STREAMS)
#endif

#if defined(USE_WITH_NTV2)
#include "ntv2status.h"
#include "ntv2utils.h"
#include "ntv2devicefeatures.h"
#endif

#if defined(USE_WITH_NTV4)
#endif

//MARK: hidden helpers

std::string storagePath(bool bSharedPrefFile)
{
	std::string path;
	
#if defined(AJA_MAC)		
	
	if (bSharedPrefFile)
	{
		path.append("/Users/Shared");
	}
	else	// Users
	{
		// get current users home dir path, /Users/<username>
		const char* homePath = getenv("HOME");
		if (homePath != NULL)
			path.append(homePath);
	}
	
	path.append("/Library/Preferences/");
	
#endif
	
#if defined(AJA_LINUX)
    // want ~/.aja/config
	
    char *home = getenv("HOME");
    if (bSharedPrefFile)
    {
        path.append("/opt/aja/config/");
    }
    else if (home)
    {
        path.append(home);
        path.append("/.aja/config/");
    }
    else
    {
        path.append("/opt/aja/config/");
    }
#endif	
	
#if defined(AJA_MAC) || defined(AJA_LINUX)
	// If the path does not exist create it
	struct stat st;
	if ( stat( path.c_str(), &st ) == -1) 
	{
		//mkdir(path.c_str(),S_IRWXU|S_IRGRP|S_IROTH); // give 744 permissions
		//mkdir(path.c_str(),S_IRWXU| S_IRWXG | S_IRWXO); // give 777 permissions
		mkdir(path.c_str(),ALLPERMS); 
		chmod(path.c_str(),ALLPERMS);
	}	
#endif	
	
#if defined(AJA_WINDOWS)
	/* makes something like \Users\<username>\AppData\Local\Aja */

	if(bSharedPrefFile)
	{
		TCHAR szPath[MAX_PATH];
		HRESULT r;
		r = SHGetFolderPath(NULL,CSIDL_COMMON_APPDATA,NULL,0,szPath);
		if(r != S_OK)
		{
			//error
		}
		else
		{
			path.erase();
#ifdef UNICODE
			PathAppend(szPath, L"Aja\\");
			char tmpPath[MAX_PATH];
			::wcstombs(tmpPath,szPath,MAX_PATH);
			path.append(tmpPath);
#else
			PathAppend(szPath, "Aja\\");
			path.append(szPath);
#endif
		}
		
	}
	else
	{
		// Need to get the user from the registry since when this is run as a service all the normal
		// calls to get the user name return SYSTEM

		// Method 1 of getting username from registry 
		char  szUserName[128];
		memset(szUserName,0,128);
		DWORD dwDataSize=128;
		HKEY hkey;
		long lResult;
		lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\LogonUI", 0, KEY_QUERY_VALUE|KEY_WOW64_64KEY, &hkey);
		if(ERROR_SUCCESS == lResult)
		{
			// read the value
			DWORD dwType;
			lResult = RegQueryValueExA(hkey,"LastLoggedOnUser", NULL, &dwType, (BYTE*)szUserName, &dwDataSize);
			RegCloseKey(hkey);
		}
		std::string tmpStr(szUserName);

		path.erase();
		path.append(getenv("SystemDrive"));
		path.append("\\Users\\");
		if(tmpStr.find('\\') != std::string::npos )
		{
			//strip off anything before a "\\"
			path.append(tmpStr.substr(tmpStr.find('\\')+1));
		}
		else
		{
			path.append(tmpStr);
		}

		//AJA_REPORT(1,AJA_DebugSeverity_Debug,"method #1 %s",path.c_str());

		//check it directory exists, if not try Method 2
		if(PathFileExistsA(path.c_str())==false)
		{
			// Method 2 of getting username from registry (will not work if logged in with a Microsoft ID)
			// http://forums.codeguru.com/showthread.php?317367-To-get-current-Logged-in-user-name-from-within-a-service
			memset(szUserName,0,128);
			lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", 0, KEY_QUERY_VALUE|KEY_WOW64_64KEY, &hkey);
			if(ERROR_SUCCESS == lResult)
			{
				// read the value
				DWORD dwType;
				lResult = RegQueryValueExA(hkey,"LastUsedUsername", NULL, &dwType, (BYTE*)szUserName, &dwDataSize);
				RegCloseKey(hkey);
			}
			path.erase();
			path.append(getenv("SystemDrive"));
			path.append("\\Users\\");
			path.append(szUserName);

			//AJA_REPORT(1,AJA_DebugSeverity_Debug,"method #2 %s",path.c_str());
		} 

		path.append("\\AppData\\Local\\Aja\\");
		//AJA_REPORT(1,AJA_DebugSeverity_Debug,"full path used %s",path.c_str());
	}

	if(PathFileExistsA(path.c_str())==false)
	{
		SHCreateDirectoryExA(NULL,path.c_str(),NULL);
	}

#endif

	return path;
}

std::string makeCreateTableString(std::string tableName,bool blobTable = false)
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

		if (isGood == false)
		{
			// try matching only keyname
			whereClause = std::string("name='") + key.c_str() + "'";
			stmt = std::string("SELECT value FROM ") + tableName + " WHERE " + whereClause + ";";
			rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);
			areResultsGood(isGood,zErrMsg,rc,nRows,azResult);
		}
	}

	char *valueAsStr=NULL;
	int valueLen = 64;

	if (isGood)
	{	
		//get value
		if(nRows > 0)
		{
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

	char colName[255];
	char value[64];
    int valueLen = 64;

    if (isGood)
    {
        //get value
        if(nRows > 0)
        {
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
	//AJADebug::Open();
	Init();
	
//TODO: temp for testing	
	std::string serialNum = "card1";

	mstateKeyName = storagePath(mSharedPrefFile);
	mstateKeyName += "board " + serialNum;
}

AJAPersistence::AJAPersistence(std::string appID, std::string deviceType, std::string deviceNumber, bool bSharePrefFile)
{
	Init();
	
	mboardId = deviceType;
	mserialNumber = deviceNumber;

	mSharedPrefFile = bSharePrefFile;
	mstateKeyName = storagePath(mSharedPrefFile);
	mstateKeyName += appID;	
}

AJAPersistence::~AJAPersistence()
{
	
}

bool AJAPersistence::SetValue(std::string key, void *value, AJAPersistenceType type, int blobSize)
{
	if(type == AJAPersistenceTypeBlob)
		return ::PersistenceSetValueBlob(mstateKeyName, key, value, blobSize, mboardId, mserialNumber);
	else
		return ::PersistenceSetValue(mstateKeyName, key, value, type, mboardId, mserialNumber);
}


bool AJAPersistence::GetValue(std::string key, void *value, AJAPersistenceType type, int blobSize)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;

	if(type == AJAPersistenceTypeBlob)
		return ::PersistenceGetValueBlob(mstateKeyName, key, value, blobSize, mboardId, mserialNumber);
	else
		return ::PersistenceGetValue(mstateKeyName, key, value, type, mboardId, mserialNumber);
}

bool AJAPersistence::GetValuesString(std::string key_query, std::vector<std::string>& keys, std::vector<std::string>& values)
{
	// with Get, don't create file if it does not exist
	if (FileExists() == false)
		return false;

	return ::PresistenceGetValues(mstateKeyName, key_query, keys, values, mboardId, mserialNumber);
}

bool AJAPersistence::GetValuesInt(std::string key_query, std::vector<std::string>& keys, std::vector<int>& values)
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

bool AJAPersistence::GetValuesBool(std::string key_query, std::vector<std::string>& keys, std::vector<bool>& values)
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

bool AJAPersistence::GetValuesDouble(std::string key_query, std::vector<std::string>& keys, std::vector<double>& values)
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
#if defined(AJA_WINDOWS)
	int val = _access(mstateKeyName.c_str(),F_OK);
#elif defined (AJA_MAC)
	int val = access(mstateKeyName.c_str(),R_OK);
#else
	int val = access(mstateKeyName.c_str(),F_OK);
#endif
	bool bExists = (val != -1);
	return bExists;
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


//MARK: NTV2 specific methods

#if defined(USE_WITH_NTV2)

AJAPersistence::AJAPersistence(CNTV2Status *card)
{
	BindToCard(card);
}

bool AJAPersistence::BindToCard(CNTV2Status *card)
{
	Init();
	
	bool isGood = false;
	if( c != NULL && c->BoardOpened() )
	{
		mpboardHandle = c;
		
		//mboardId 		 = mpboardHandle->GetBoardID();
		
		std::string serialNum = "";
		mpboardHandle->GetSerialNumberString(serialNum);
		
		mstateKeyName = storagePath(mIsShared);
		mstateKeyName += "board " + serialNum;

		isGood = SynchronizeState();			
	}
	
	return isGood;
}

#endif //USE_NTV2

//MARK: Private Methods

void AJAPersistence::Init()
{
    mboardId        = "";
    mserialNumber	= "";
    mSharedPrefFile = false;
}

void dropTables(std::string dbPath)
{
	std::string stmt;
	sqlite3 *db;	
	int rc=0;
	char *zErrMsg = 0;
	char **azResult;
	int nRows;
	int nCols;

	rc = sqlite3_open(dbPath.c_str(), &db);
	if(rc != SQLITE_OK)
	{
		sqlite3_close(db);
		return;
	}

	stmt = std::string("DROP TABLE persistence;");
	rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);
	sqlite3_free(zErrMsg);zErrMsg=0;
	sqlite3_free_table(azResult);

	stmt = std::string("DROP TABLE persistenceBlobs;");
	rc = sqlite3_get_table(db,stmt.c_str(),&azResult,&nRows,&nCols,&zErrMsg);
	sqlite3_free(zErrMsg);zErrMsg=0;
	sqlite3_free_table(azResult);

	sqlite3_close(db);
}

bool AJAPersistence::UnitTestDiskReadWrite()
{
	std::string keyName     = "";
	int			intValue    = 42;
	bool		trueValue   = true;
	bool		falseValue  = false;
	double		doubleValue = 3.14;
	std::string strValue    = "testing 1,2,3";
	char		blobValue[] = "blob test data";
	int			blobLen     = (int)strlen(blobValue);
	int			hierarchyValue1 = 17;
	int			hierarchyValue2 = 23;
	int			hierarchyValue3 = 27;

	std::string longStrValue = "some really long string to test if text stored as values are getting clipped in the persistence storage. like I said this is a long string testing the ability of sqlite to handle long strings of text. even though the column is set to 64 chars, mysql lets it grow to fit longer strings.";
	std::string orgLongStrValue = longStrValue;

	bool		isGood;
	
	//clear out any old values
	dropTables(mstateKeyName);

	// Write
	mboardId = "";
	mserialNumber = "";
	// int
	keyName = "UnitTestInt";
	SetValue(keyName, &intValue, AJAPersistenceTypeInt);
	intValue = 0;
	
	// bool
	keyName = "UnitTestBoolTrue";
	SetValue(keyName, &trueValue, AJAPersistenceTypeBool);
	trueValue = false;
	
	keyName = "UnitTestBoolFalse";
	SetValue(keyName, &falseValue, AJAPersistenceTypeBool);
	falseValue = true;
	
	// float
	keyName = "UnitTestDouble";
	SetValue(keyName, &doubleValue, AJAPersistenceTypeDouble);
	doubleValue = 0.0;
	
	// string
	keyName = "UnitTestString";			
	SetValue(keyName, &strValue, AJAPersistenceTypeString);
	strValue = "";
	
	// blob
	keyName = "UnitTestBlob";
	SetValue(keyName, &blobValue, AJAPersistenceTypeBlob, blobLen);
	strcpy(blobValue,"");
	memset(blobValue,0,blobLen);

	// long string
	keyName = "UnitTestString (long)";
	SetValue(keyName, &longStrValue, AJAPersistenceTypeString);
	longStrValue = "";

	// write values to test hierarchical search
	keyName = "UnitTestHierarchyInt";
	mboardId = "";
	mserialNumber = "";
	SetValue(keyName, &hierarchyValue1, AJAPersistenceTypeInt);
	hierarchyValue1 = 0;

	keyName = "UnitTestHierarchyInt";
	mboardId = "device 1";
	mserialNumber = "123456";
	SetValue(keyName, &hierarchyValue2, AJAPersistenceTypeInt);
	hierarchyValue2 = 0;

	keyName = "UnitTestHierarchyInt";
	mboardId = "device 2";
	mserialNumber = "987654";
	SetValue(keyName, &hierarchyValue3, AJAPersistenceTypeInt);
	hierarchyValue3 = 0;

	// Read
	mboardId = "";
	mserialNumber = "";

	// int
	keyName = "UnitTestInt";	
	isGood = GetValue(keyName, &intValue, AJAPersistenceTypeInt);		
	
	// bool
	keyName = "UnitTestBoolTrue";	
	isGood = GetValue(keyName, &trueValue, AJAPersistenceTypeBool);	
	
	keyName = "UnitTestBoolFalse";
	isGood = GetValue(keyName, &falseValue, AJAPersistenceTypeBool);		
	
	// float
	keyName = "UnitTestDouble";
	isGood = GetValue(keyName, &doubleValue, AJAPersistenceTypeDouble);
	
	// string
	keyName = "UnitTestString";	
	isGood = GetValue(keyName, &strValue, AJAPersistenceTypeString);
	
	// blob
	keyName = "UnitTestBlob";
	isGood = GetValue(keyName, &blobValue, AJAPersistenceTypeBlob, blobLen);

	// long string
	keyName = "UnitTestString (long)";
	isGood = GetValue(keyName, &longStrValue, AJAPersistenceTypeString);

	// test hierarchical search
	keyName = "UnitTestHierarchyInt";
	mboardId = "device 3";		//not in db
	mserialNumber = "424242";	//not in db
	isGood = GetValue(keyName, &hierarchyValue1, AJAPersistenceTypeInt);

	mboardId = "device 1";		//in db
	mserialNumber = "171717";	//not in db
	isGood = GetValue(keyName, &hierarchyValue2, AJAPersistenceTypeInt);

	mboardId = "device 2";		//in db
	mserialNumber = "987654";	//in db
	isGood = GetValue(keyName, &hierarchyValue3, AJAPersistenceTypeInt);

	if(intValue == 42 &&
	   trueValue == true &&
	   falseValue == false &&
	   doubleValue > 3.1 && doubleValue < 3.15 &&
	   strValue == "testing 1,2,3" &&
	   strcmp(blobValue,"blob test data")==0 && 
	   longStrValue == orgLongStrValue &&
	   hierarchyValue1 == 17 &&					//should be 17 since device and serial not found
	   hierarchyValue2 == 23 &&					//should be 23 since device found but serial not found
	   hierarchyValue3 == 27 					//should be 27 since an exact match
	   )
	{
		return true;
	}
	else
	{
		return false;
	}

}
