/**
	@file		windows/file_io.cpp
	@copyright	Copyright (C) 2011-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJAFileIO class on the Windows platform.
**/

#include "ajabase/common/common.h"
#include "ajabase/system/file_io.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>

using std::string;
using std::wstring;
using std::vector;

// found at: http://www.frenk.com/2009/12/convert-filetime-to-unix-timestamp/
int64_t FileTime_to_POSIX(FILETIME ft)
{
	// takes the last modified date
	LARGE_INTEGER date, adjust;
	date.HighPart = ft.dwHighDateTime;
	date.LowPart = ft.dwLowDateTime;
	 
	// 100-nanoseconds = milliseconds * 10000
	adjust.QuadPart = 11644473600000 * 10000;
	 
	// removes the diff between 1970 and 1601
	date.QuadPart -= adjust.QuadPart;
	 
	// converts back from 100-nanoseconds to seconds
	return (int64_t)(date.QuadPart / 10000000);
}

bool
AJAFileIO::FileExists(const wstring& fileName)
{
	struct _stat dummy;
	return _wstat(fileName.c_str(), &dummy) != -1;
}

bool
AJAFileIO::FileExists(const string& fileName)
{
	struct _stat dummy;
	return _stat(fileName.c_str(), &dummy) != -1;
}

AJAFileIO::AJAFileIO(void)
{
	mFileDescriptor = INVALID_HANDLE_VALUE;
}

AJAFileIO::~AJAFileIO(void)
{
	Close();
}

AJAStatus
AJAFileIO::Open(
	const string&			fileName,
	const int				flags,
	const int				properties)
{
	wstring wString;
    aja::string_to_wstring(fileName,wString);
	AJAStatus status = Open(wString,flags,properties);

	return status;
}

AJAStatus
AJAFileIO::Open(
	const wstring&			fileName,
	const int				flags,
	const int				properties)
{
	DWORD     desiredAccess       = 0;
	DWORD     creationDisposition = 0;
	DWORD     flagsAndAttributes  = 0;
	DWORD	  shareMode			  = 0;
	AJAStatus status              = AJA_STATUS_FAIL;

	if ((INVALID_HANDLE_VALUE == mFileDescriptor) &&
		(0 != fileName.length()))
	{
		// If the flags are not compatable, we will let
		// Windows provide the error checking.
		if ((eAJAReadOnly & flags) || (eAJAReadWrite & flags))  //  O_RDONLY, O_RDWR
		{
			desiredAccess |= GENERIC_READ;
			shareMode = FILE_SHARE_READ;
		}
		if ((eAJAWriteOnly & flags) || (eAJAReadWrite & flags)) //  O_WRONLY
        {
			desiredAccess |= GENERIC_WRITE;
            shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        }

		if (eAJACreateAlways & flags)
			creationDisposition |= CREATE_ALWAYS;     //  O_CREAT
		if (eAJACreateNew & flags)
			creationDisposition |= CREATE_NEW;        // (O_CREAT || O_EXCL)
		if (eAJATruncateExisting & flags)
			creationDisposition |= TRUNCATE_EXISTING; //  O_TRUNC
		if (eAJAReadOnly & flags)
			creationDisposition |= OPEN_EXISTING;

		if (eAJAUnbuffered & properties)
			flagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
		
		mFileDescriptor = CreateFileW(
							fileName.c_str(),
							desiredAccess,
							shareMode, 
							NULL,
							creationDisposition,
							flagsAndAttributes,
							NULL);


		if (INVALID_HANDLE_VALUE != mFileDescriptor)
		{
			status = AJA_STATUS_SUCCESS;
		}
	}
	return (status);
}


AJAStatus
AJAFileIO::Close(void)
{
	AJAStatus status = AJA_STATUS_FAIL;

	if (INVALID_HANDLE_VALUE != mFileDescriptor)
	{
		if (TRUE == CloseHandle(mFileDescriptor))
		{
			status = AJA_STATUS_SUCCESS;
		}
		mFileDescriptor = INVALID_HANDLE_VALUE;
	}
	return (status);
}


bool
AJAFileIO::IsOpen(void)
{
	return (INVALID_HANDLE_VALUE != mFileDescriptor);
}


uint32_t
AJAFileIO::Read(uint8_t* pBuffer, const uint32_t length)
{
	DWORD bytesRead = 0;

	if (INVALID_HANDLE_VALUE != mFileDescriptor)
	{
		ReadFile(mFileDescriptor, pBuffer, length, &bytesRead, NULL);
	}
	return (bytesRead);
}


uint32_t
AJAFileIO::Write(const uint8_t* pBuffer, const uint32_t length) const
{
	DWORD bytesWritten = 0;

	if (INVALID_HANDLE_VALUE != mFileDescriptor)
	{
		WriteFile(mFileDescriptor, pBuffer, length, &bytesWritten, NULL);
	}
	return (bytesWritten);
}


uint32_t
AJAFileIO::Write(const string& buffer) const
{
	return (uint32_t)(Write((uint8_t*) buffer.c_str(), (uint32_t)buffer.length()));
}


void
AJAFileIO::Sync(void)
{
}


int64_t
AJAFileIO::Tell(void)
{
	int64_t retVal = 0;

	if (INVALID_HANDLE_VALUE != mFileDescriptor)
	{
		LARGE_INTEGER liDistanceToMove;
		liDistanceToMove.HighPart = 0;
		liDistanceToMove.LowPart = 0;

		LARGE_INTEGER liCurrentFilePointer;

		BOOL status = SetFilePointerEx(mFileDescriptor, liDistanceToMove, &liCurrentFilePointer, FILE_CURRENT);
		
		if (status == 0)
		{
			retVal = (int64_t)-1;
		}
		else
		{
			retVal = liCurrentFilePointer.QuadPart;
		}
	}
	return retVal;
}


AJAStatus
AJAFileIO::Seek(const int64_t distance, const AJAFileSetFlag flag) const
{
	DWORD     moveMethod;
	DWORD     retVal;
	AJAStatus status = AJA_STATUS_FAIL;

	if (INVALID_HANDLE_VALUE != mFileDescriptor)
	{
		switch (flag)
		{
			case eAJASeekSet:
				moveMethod = FILE_BEGIN;
				break;

			case eAJASeekCurrent:
				moveMethod = FILE_CURRENT;
				break;

			case eAJASeekEnd:
				moveMethod = FILE_END;
				break;

			default:
				return (AJA_STATUS_BAD_PARAM);
		}
		LARGE_INTEGER liDistanceToMove;
		liDistanceToMove.HighPart = (LONG)(distance>>32);
		liDistanceToMove.LowPart = (DWORD)distance;

		retVal = SetFilePointerEx(mFileDescriptor, liDistanceToMove, NULL, moveMethod);
		if ( retVal	== TRUE )
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

	if(IsOpen())
	{
		FILETIME cTime;
		FILETIME aTime;
		FILETIME wTime;
		if( GetFileTime(mFileDescriptor,&cTime,&aTime,&wTime) )
		{
			LARGE_INTEGER sizeInfo;
			if(GetFileSizeEx(mFileDescriptor,&sizeInfo))
			{
				size = (int64_t)sizeInfo.QuadPart;
				createTime = FileTime_to_POSIX(cTime);
				modTime = FileTime_to_POSIX(wTime);
				status = AJA_STATUS_SUCCESS;
			}
		}
	}

	return status;
}

AJAStatus
AJAFileIO::Delete(const string& fileName)
{
	AJAStatus status = AJA_STATUS_FAIL;

	if (0 != fileName.length())
	{
		if (DeleteFileA(fileName.c_str()))
		{
			status = AJA_STATUS_SUCCESS;
		}
	}
	return (status);
}

AJAStatus
AJAFileIO::Delete(const wstring& fileName)
{
	AJAStatus status = AJA_STATUS_FAIL;

	if (0 != fileName.length())
	{
		if (DeleteFileW(fileName.c_str()))
		{
			status = AJA_STATUS_SUCCESS;
		}
	}
	return (status);
}

AJAStatus
AJAFileIO::ReadDirectory(
				const string&   directory,
				const string&   filePattern,
				vector<string>& fileContainer)
{
	WIN32_FIND_DATAA fileData;
	HANDLE           hSearch;
	string           qualifiedName;
	AJAStatus        status = AJA_STATUS_FAIL;

	fileContainer.clear();

	if ((0 != directory.length()) && (0 != filePattern.length()))
	{
		if (TRUE == SetCurrentDirectoryA(directory.c_str()))
		{
			if (INVALID_HANDLE_VALUE !=
				(hSearch = FindFirstFileA(filePattern.c_str(), &fileData)))
			{
				qualifiedName = directory + "/" + fileData.cFileName;
				fileContainer.push_back(qualifiedName);
	
				while (FindNextFileA(hSearch, &fileData) != 0)
				{
					qualifiedName = directory + "/" + fileData.cFileName;
					fileContainer.push_back(qualifiedName);
				}
				FindClose(hSearch);

				if (0 != fileContainer.size())
				{
					status = AJA_STATUS_SUCCESS;
				}
			}
		}
	}
	return (status);
}

AJAStatus
AJAFileIO::ReadDirectory(
				const wstring&   directory,
				const wstring&   filePattern,
				vector<wstring>& fileContainer)
{
	WIN32_FIND_DATAW fileData;
	HANDLE           hSearch;
	wstring          qualifiedName;
	AJAStatus        status = AJA_STATUS_FAIL;

	fileContainer.clear();

	if ((0 != directory.length()) && (0 != filePattern.length()))
	{
		if (TRUE == SetCurrentDirectoryW(directory.c_str()))
		{
			if (INVALID_HANDLE_VALUE !=
				(hSearch = FindFirstFileW(filePattern.c_str(), &fileData)))
			{
				qualifiedName = directory + L"/" + fileData.cFileName;
				fileContainer.push_back(qualifiedName);
	
				while (FindNextFileW(hSearch, &fileData) != 0)
				{
					qualifiedName = directory + L"/" + fileData.cFileName;
					fileContainer.push_back(qualifiedName);
				}
				FindClose(hSearch);

				if (0 != fileContainer.size())
				{
					status = AJA_STATUS_SUCCESS;
				}
			}
		}
	}
	return (status);
}


AJAStatus
AJAFileIO::DoesDirectoryContain(
				const string& directory,
				const string& filePattern)
{
	WIN32_FIND_DATAA fileData;
	HANDLE           hSearch;
	char             savePath[MAX_PATH+1];
	AJAStatus        status = AJA_STATUS_FAIL;

	if ((0 != directory.length()) && (0 != filePattern.length()))
	{
		if( !GetCurrentDirectoryA(MAX_PATH, savePath) )
			return (status);

		if (TRUE == SetCurrentDirectoryA(directory.c_str()))
		{
			if (INVALID_HANDLE_VALUE !=
				(hSearch = FindFirstFileA(filePattern.c_str(), &fileData)))
			{
				FindClose(hSearch);

				status = AJA_STATUS_SUCCESS;
			}

			SetCurrentDirectoryA(savePath);
		}
	}
	return (status);
}

AJAStatus
AJAFileIO::DoesDirectoryContain(
				const wstring& directory,
				const wstring& filePattern)
{
	WIN32_FIND_DATAW fileData;
	HANDLE           hSearch;
	wchar_t          savePath[MAX_PATH+1];
	AJAStatus        status = AJA_STATUS_FAIL;

	if ((0 != directory.length()) && (0 != filePattern.length()))
	{
		if( !GetCurrentDirectoryW(MAX_PATH, savePath) )
			return (status);

		if (TRUE == SetCurrentDirectoryW(directory.c_str()))
		{
			if (INVALID_HANDLE_VALUE !=
				(hSearch = FindFirstFileW(filePattern.c_str(), &fileData)))
			{
				FindClose(hSearch);

				status = AJA_STATUS_SUCCESS;
			}

			SetCurrentDirectoryW(savePath);
		}
	}
	return (status);
}


AJAStatus
AJAFileIO::DoesDirectoryExist(const string& directory)
{
	return( (::GetFileAttributesA(directory.c_str()) != INVALID_FILE_ATTRIBUTES) ? AJA_STATUS_SUCCESS : AJA_STATUS_FAIL);
}

AJAStatus
AJAFileIO::DoesDirectoryExist(const wstring& directory)
{
	return( (::GetFileAttributesW(directory.c_str()) != INVALID_FILE_ATTRIBUTES) ? AJA_STATUS_SUCCESS : AJA_STATUS_FAIL);
}

AJAStatus
AJAFileIO::IsDirectoryEmpty(const string& directory)
{
	return( DoesDirectoryContain(directory, "*") );
}

AJAStatus
AJAFileIO::IsDirectoryEmpty(const wstring& directory)
{
	return( DoesDirectoryContain(directory, L"*") );
}
