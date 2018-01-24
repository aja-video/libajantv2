/**
	@file		file_io.h
	@copyright	Copyright (C) 2011-2018 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJAFileIO class.
**/

#ifndef AJA_FILE_IO_H
#define AJA_FILE_IO_H


#include "ajabase/common/types.h"
#include "ajabase/common/public.h"
#include "ajabase/system/system.h"
#include <vector>
#include <string>

typedef enum
{
	eAJACreateAlways     = 1,
	eAJACreateNew        = 2,
	eAJATruncateExisting = 4,

	eAJAReadOnly         = 8,
	eAJAWriteOnly        = 16,
	eAJAReadWrite        = 32
} AJAFileCreationFlags;


typedef enum
{
	eAJABuffered		 = 1,
	eAJAUnbuffered		 = 2,
	eAJANoCaching		 = 4
} AJAFileProperties;


typedef enum
{
	eAJASeekSet,
	eAJASeekCurrent,
	eAJASeekEnd
} AJAFileSetFlag;


/**
 *	The File I/O class proper.
 *	@ingroup AJAGroupSystem
 */
class  AJA_EXPORT AJAFileIO
{
public:
	AJAFileIO(void);
	~AJAFileIO(void);

	/**
	 *	Open a file.
	 *
	 *	@param[in]	fileName			The fully qualified file name
	 *	@param[in]	flags				The way in which the file is opened
	 *	@param[in]	properties			Indicates whether the file is buffered or not
	 *
	 *	@return		AJA_STATUS_SUCCESS	A file has been successfully opened
	 *				AJA_STATUS_FAIL		A file could not be opened
	 */
	AJAStatus Open(
				const std::string &		fileName,
				const int				flags,
				const int				properties);

	AJAStatus Open(
				const std::wstring &	fileName,
				const int				flags,
				const int				properties);

	/**
	 *	Close a file.
	 *
	 *	@return		AJA_STATUS_SUCCESS	The file was successfully closed
	 *				AJA_STATUS_FAIL		The file could not be closed
	 */
	AJAStatus Close(void);

	/**
	 *	Tests for a valid open file.
	 *
	 *	@return		bool				'true' if a valid file is available
	 */
	bool IsOpen(void);

	/**
	 *	Read the contents of the file.
	 *
	 *	@param[in]	pBuffer				The buffer to be written to
	 *	@param[in]	length				The number of bytes to be read
	 *
	 *	@return		uint32_t			The number of bytes actually read
	 */
	uint32_t Read(uint8_t* pBuffer, const uint32_t length);
	
	/**
	 *	Write the contents of the file.
	 *
	 *	@param[in]	pBuffer				The buffer to be written out
	 *	@param[in]	length				The number of bytes to be written
	 *
	 *	@return		uint32_t			The number of bytes actually written
	 */	
	uint32_t Write(const uint8_t* pBuffer, const uint32_t length) const;

	/**
	 *	Write the contents of the file.
	 *
	 *	@param[in]	buffer				The buffer to be written out
	 *
	 *	@return		uint32_t			The number of bytes actually written
	 */
	uint32_t Write(const std::string& buffer) const;

	/**
	 *	Flush the cache 
	 *
	 */	
	void Sync(void);

	/**
	 *	Truncates the file.
	 *
	 *	@param[in]	offset			The size offset of the file
	 */
	void Truncate(int32_t offset);

	/**
	 *	Retrieves the offset of the file pointer from the start of a file.
	 *
	 *	@return		int64_t			The position of the file pointer, -1 if error
	 */
	int64_t Tell(void);

	/**
	 *	Moves the offset of the file pointer.
	 *
	 *	@param[in]	distance			The distance to move the file pointer
	 *	@param[in]	flag				Describes from whence to move the file pointer
	 *
	 *	@return		AJA_STATUS_SUCCESS	The position of the file pointer was moved
	 */
	AJAStatus Seek(const int64_t distance, const AJAFileSetFlag flag) const;

	/**
	 *	Get some basic file info
	 *
	 *	@param[out]	createTime			Time of file creation, measured in seconds since 1970
	 *	@param[out]	modTime				Last time file was modified, measured in seconds since 1970
	 *	@param[out]	size				Size of the file in bytes
	 *
	 *	@return		AJA_STATUS_SUCCESS	Was able to get info from the file
	 */
	AJAStatus FileInfo(int64_t& createTime, int64_t& modTime, int64_t& size);

    /**
     *	Test file to see if it exists
     *
     *	@param[in]	fileName			The fully qualified file name
     *
     *	@return		bool				true if file exists
     */
    static bool FileExists(const std::wstring& fileName);
    static bool FileExists(const std::string& fileName);

	/**
	 *	Remove the file for the system
	 *
	 *	@param[in]	fileName			The fully qualified file name
	 *
	 *	@return		AJA_STATUS_SUCCESS	The file was successfully deleteed
	 *				AJA_STATUS_FAIL		The file could not be deleted
	 */
    static AJAStatus Delete(const std::string& fileName);
    static AJAStatus Delete(const std::wstring& fileName);

	/**
	 *	Retrieves a set of files from a directory.
	 *	Changes the current directory.
	 *
	 *	@param[in]	directory			The path to the directory
	 *	@param[in]	filePattern			The pattern within the directory to match
	 *	@param[out]	fileContainer		The files that match the file pattern
	 *
	 *	@return		AJA_STATUS_SUCCESS	The returned container has a size > 0 
	 */
    static AJAStatus ReadDirectory(
				const std::string&   directory,
				const std::string&   filePattern,
				std::vector<std::string>& fileContainer);

    static AJAStatus ReadDirectory(
				const std::wstring&   directory,
				const std::wstring&   filePattern,
				std::vector<std::wstring>& fileContainer);

	/**
	 *	Tests if a directory contains a file that matches the pattern.
	 *	Does not change the current directory.
	 *
	 *	@param[in]	directory			The path to the directory
	 *	@param[in]	filePattern			The pattern within the directory to match
	 *
	 *	@return		AJA_STATUS_SUCCESS	If the directory has at least one matching file
	 */
    static AJAStatus DoesDirectoryContain(
				const std::string& directory,
				const std::string& filePattern);

    static AJAStatus DoesDirectoryContain(
				const std::wstring& directory,
				const std::wstring& filePattern);

	/**
	 *	Tests if a directory exists.
	 *	Does not change the current directory.
	 *
	 *	@param[in]	directory			The path to the directory
	 *
	 *	@return		AJA_STATUS_SUCCESS	If and only if the directory exists
	 */
    static AJAStatus DoesDirectoryExist(const std::string& directory);
    static AJAStatus DoesDirectoryExist(const std::wstring& directory);

	/**
	 *	Tests if a directory is empty.
	 *	Does not change the current directory.
	 *
	 *	@param[in]	directory			The path to the directory
	 *
	 *	@return		AJA_STATUS_SUCCESS	If and only if the directory contains no files
	 */
    static AJAStatus IsDirectoryEmpty(const std::string& directory);
    static AJAStatus IsDirectoryEmpty(const std::wstring& directory);

#if defined(AJA_LINUX) || defined(AJA_MAC)
	void     *GetHandle(void) {return NULL;}
#elif defined(AJA_WINDOWS)
	void     *GetHandle(void) {return mFileDescriptor;}
#endif

private:

#if defined(AJA_LINUX) || defined(AJA_MAC)
	FILE*        mpFile;
    int          mFileDescriptor;
#elif defined(AJA_WINDOWS)
    HANDLE       mFileDescriptor;
#endif
};

#endif // AJA_FILE_IO_H
