/**
	@file		linux/file_io.cpp
	@copyright	Copyright (C) 2011-2018 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJAFileIO class on the Linux platform.
**/

#include "ajabase/common/common.h"
#include "ajabase/system/file_io.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>
#include <limits.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

using std::string;
using std::wstring;
using std::vector;

AJAFileIO::AJAFileIO(void)
{
	mpFile          = NULL;
	mFileDescriptor = -1;
}


AJAFileIO::~AJAFileIO(void)
{
	Close();
}

bool
AJAFileIO::FileExists(const std::wstring& fileName)
{
	string aString;
    aja::wstring_to_string(fileName, aString);
	return FileExists(aString);
}

bool
AJAFileIO::FileExists(const std::string& fileName)
{
	struct stat dummy;
	bool bExists = stat(fileName.c_str(), &dummy) != -1;
	return bExists;
}

AJAStatus
AJAFileIO::Open(
    const std::wstring&   fileName,
    int flags,
    int properties)
{
    string aString;
    aja::wstring_to_string(fileName,aString);
    AJAStatus status = Open(aString,flags,properties);

    return status;
}

AJAStatus
AJAFileIO::Open(
	const std::string&		fileName,
	const int				flags,
	const int	properties)
{
	AJAStatus status             = AJA_STATUS_FAIL;
	string    flagsAndAttributes;

	if ((-1 == mFileDescriptor) &&
		(0 != fileName.length()))
	{
		// If the flags are not compatable, we will let
		// Linux provide the error checking.
		if (eAJAReadOnly & flags)
		{
			flagsAndAttributes = "r";
		}
		else if (eAJAWriteOnly & flags)
		{
			if (eAJATruncateExisting & flags)
			{
				flagsAndAttributes = "w";
			}
			else
			{
				flagsAndAttributes = "w+";
			}
		}
		else if (eAJAReadWrite & flags)
		{
			if (eAJATruncateExisting & flags)
			{
				flagsAndAttributes = "w+";
			}
			else
			{
				if (eAJACreateAlways & flags)
				{
					flagsAndAttributes = "a+";
				}
				if (eAJACreateNew & flags)
				{
					flagsAndAttributes = "w+";
				}
			}
		}

		if (true == flagsAndAttributes.empty())
		{
			return (AJA_STATUS_BAD_PARAM);
		}

		// One can also change the buffering behavior via:
		// setvbuf(FILE*, char* pBuffer, _IOFBF,  size_t size);
		mpFile = fopen(
					fileName.c_str(),
					flagsAndAttributes.c_str()); 

		if (NULL != mpFile)
		{
			if (eAJAUnbuffered & properties)
			{
				if (-1 != (mFileDescriptor = fileno(mpFile)))
				{
					status = AJA_STATUS_SUCCESS;
				}
			}
			else
			{
				status = AJA_STATUS_SUCCESS;
			}
		}
	}
	return (status);
}


AJAStatus
AJAFileIO::Close(void)
{
	AJAStatus status = AJA_STATUS_FAIL;

	if (NULL != mpFile)
	{
		if (0 == fclose(mpFile))
		{
			status = AJA_STATUS_SUCCESS;
		}
		mpFile          = NULL;
		mFileDescriptor = -1;
	}
	return (status);
}


bool
AJAFileIO::IsOpen(void)
{
	return (NULL != mpFile);
}


uint32_t
AJAFileIO::Read(uint8_t* pBuffer, const uint32_t length)
{
	uint32_t retVal = 0;

	if (-1 != mFileDescriptor)
	{
		ssize_t bytesRead = 0;

		if ((bytesRead = read(mFileDescriptor, pBuffer, length)) > 0)
		{
			retVal = uint32_t(bytesRead);
		}
	}
	else if (NULL != mpFile)
	{
		size_t bytesRead = 0;

		if ((bytesRead = fread(pBuffer, length, 1, mpFile)) > 0)
		{
			retVal = uint32_t(length);
		}
	}
	return (retVal);
}


uint32_t
AJAFileIO::Write(const uint8_t* pBuffer, const uint32_t length) const
{
	uint32_t retVal = 0;

	if (-1 != mFileDescriptor)
	{
		ssize_t bytesWritten = 0;

		if ((bytesWritten = write(mFileDescriptor, pBuffer, length)) > 0)
		{
			retVal = uint32_t(bytesWritten);
		}
	}
	else if (NULL != mpFile)
	{
		size_t bytesWritten = 0;

		if ((bytesWritten = fwrite(pBuffer, length, 1, mpFile)) > 0)
		{
			retVal = uint32_t(length);
		}
	}
	return (retVal);
}



uint32_t
AJAFileIO::Write(const std::string& buffer) const
{
	return (Write((uint8_t*) buffer.c_str(), buffer.length()));
}


void
AJAFileIO::Sync(void)
{
	if (-1 != mFileDescriptor)
	{
		fsync(mFileDescriptor);
	}
}


int64_t
AJAFileIO::Tell(void)
{
	return (int64_t)ftello(mpFile);
}


AJAStatus
AJAFileIO::Seek(const int64_t distance, const AJAFileSetFlag flag) const
{
	AJAStatus status = AJA_STATUS_FAIL;
	int       whence;
	long int  retVal;

	if (NULL != mpFile)
	{
		switch (flag)
		{
			case eAJASeekSet:
				whence = SEEK_SET;
				break;

			case eAJASeekCurrent:
				whence = SEEK_CUR;
				break;

			case eAJASeekEnd:
				whence = SEEK_END;
				break;

			default:
				return (AJA_STATUS_BAD_PARAM);
		}
		retVal = fseeko(mpFile, (off_t)distance, whence);

		if (-1 != retVal)
		{
			status = AJA_STATUS_SUCCESS;
		}
	}
	return (status);
}

AJAStatus 
AJAFileIO::FileInfo(int64_t& createTime, int64_t& modTime, int64_t& size)
{
	AJAStatus status = AJA_STATUS_FAIL;
	
	createTime = modTime = size = 0;

	if( NULL != mpFile)
	{
		struct stat64 fileStatus;
		int fErr = fstat64(fileno(mpFile),&fileStatus);

		if (fErr == 0)
		{
			size = fileStatus.st_size;
			createTime = fileStatus.st_ctime;
			modTime = fileStatus.st_mtime;

			status = AJA_STATUS_SUCCESS;
		}
	}

	return status;
}

AJAStatus
AJAFileIO::Delete(const std::string& fileName)
{
	AJAStatus status = AJA_STATUS_FAIL;

	if (0 != fileName.length())
	{
		if (0 == unlink(fileName.c_str()))
		{
			status = AJA_STATUS_SUCCESS;
		}
	}
	return (status);
}

AJAStatus
AJAFileIO::Delete(const std::wstring& fileName)
{
    AJAStatus status = AJA_STATUS_FAIL;

    string aString;
    aja::wstring_to_string(fileName,aString);
    status = Delete(aString);

    return status;
}

AJAStatus
AJAFileIO::ReadDirectory(
				const std::string&   directory,
				const std::string&   filePattern,
				std::vector<std::string>& fileContainer)
{
	AJAStatus       status = AJA_STATUS_FAIL;
	struct dirent** ppNamelist;
	int             nEntries;
	string          fileEntry;
	string          convertedPath;
	string			upperPattern;
	char            resolvedPath[PATH_MAX];

	if ((0 != directory.length()) && (0 != filePattern.length()))
	{
		// Convert any Windows path chars to Linux
		convertedPath = directory;
		for (string::iterator it = convertedPath.begin();
				it < convertedPath.end();
                ++it)
		{
			if( *it == '\\' )
				*it = '/';
		}

		// Force the pattern to upper case
		upperPattern = filePattern;
		for (string::iterator it = upperPattern.begin();
				it < upperPattern.end();
                ++it)
		{
			*it = toupper( *it );
		}

		// Make sure directory path is cleaned up
		if (!realpath(convertedPath.c_str(), resolvedPath))
			return (status);  // Path is bad

		nEntries = scandir(resolvedPath, &ppNamelist, 0, versionsort);

		if (nEntries > 0)
		{
			for (int ndx = 0; ndx < nEntries; ndx++)
			{
				char* pName = ppNamelist[ndx]->d_name;

				// Make an upper case copy of the file name
				char upperName[PATH_MAX];
				char* pChar = pName;
				int length = strlen( pName );
				int i;
				for (i = 0; i < length; i++) 
				{
					upperName[i] = toupper( *pChar++ );
				}
				upperName[i] = '\0';

				if (!fnmatch(upperPattern.c_str(), upperName, FNM_PERIOD))
				{
					fileEntry  = (directory + "/");
					fileEntry += pName;

					fileContainer.push_back(fileEntry);
				}
			}
			free(ppNamelist);
			status = AJA_STATUS_SUCCESS;
		}
	}
	return (status);
}

AJAStatus
AJAFileIO::ReadDirectory(
                const std::wstring&   directory,
                const std::wstring&   filePattern,
                std::vector<std::wstring>& fileContainer)
{
    AJAStatus status = AJA_STATUS_FAIL;

    string aDir,aPat;
    aja::wstring_to_string(directory,aDir);
    aja::wstring_to_string(filePattern,aPat);
    vector<string> aContainer;
    status = ReadDirectory(aDir,aPat,aContainer);
    for(vector<string>::iterator i = aContainer.begin(); i != aContainer.end(); ++i)
    {
        wstring tmp;
        aja::string_to_wstring(*i,tmp);
        fileContainer.push_back(tmp);
    }

    return status;
}

AJAStatus
AJAFileIO::DoesDirectoryContain(
				const std::string& directory,
				const std::string& filePattern)
{
	AJAStatus       status = AJA_STATUS_FAIL;
	vector<string>  fileList;

	if ((0 != directory.length()) && (0 != filePattern.length()))
	{
		AJAStatus readStatus = ReadDirectory( directory, filePattern, fileList );
		if( readStatus == AJA_STATUS_SUCCESS )
		{
			if( fileList.size() >= 2 )	// Don't count "." and ".."
				status = AJA_STATUS_SUCCESS;
			else
				status = AJA_STATUS_FAIL;
		}
	}
	return (status);
}

AJAStatus
AJAFileIO::DoesDirectoryContain(
                const std::wstring& directory,
                const std::wstring& filePattern)
{
    AJAStatus status = AJA_STATUS_FAIL;
    string aDir,aPat;
    aja::wstring_to_string(directory,aDir);
    aja::wstring_to_string(filePattern,aPat);
    status = DoesDirectoryContain(aDir,aPat);

    return status;
}


AJAStatus
AJAFileIO::DoesDirectoryExist(const std::string& directory)
{
	AJAStatus status = AJA_STATUS_FAIL;

	if (0 != directory.length())
	{
		DIR* pDir = opendir( directory.c_str() );
		if( pDir )
		{
			closedir( pDir );
			status = AJA_STATUS_SUCCESS;
		}
	}
	return (status);
}

AJAStatus
AJAFileIO::DoesDirectoryExist(const std::wstring& directory)
{
    AJAStatus status = AJA_STATUS_FAIL;
    string aDir;
    aja::wstring_to_string(directory,aDir);
    status = DoesDirectoryExist(aDir);

    return status;
}

AJAStatus
AJAFileIO::IsDirectoryEmpty(const std::string& directory)
{
	if (AJA_STATUS_FAIL ==  DoesDirectoryContain(directory, "*"))
		return AJA_STATUS_SUCCESS;
	else
		return AJA_STATUS_FAIL;
}

AJAStatus
AJAFileIO::IsDirectoryEmpty(const std::wstring& directory)
{
    return( DoesDirectoryContain(directory, L"*") );
}

