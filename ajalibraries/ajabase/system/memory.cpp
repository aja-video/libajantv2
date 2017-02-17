/**
	@file		memory.cpp
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJAMemory class.
**/

#if defined(AJA_LINUX) || defined(AJA_MAC)
    #include <errno.h>
    #include <fcntl.h>
    #include <stdlib.h>
    #include <syslog.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif

#include "ajabase/system/system.h"
#include "ajabase/common/common.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/lock.h"

// structure to track shared memory allocations
struct SharedData
{
	std::string	shareName;
	void*		pMemory;
	size_t		memorySize;
	int32_t		refCount;
#if defined(AJA_WINDOWS)
	HANDLE		fileMapHandle;
#endif
#if defined(AJA_LINUX) || defined(AJA_MAC)
	int			fileDescriptor;
#endif
};

// lock for shared memory allocation/free
static AJALock sSharedLock;

// list of allocated shared memory
static std::list<SharedData> sSharedList;



AJAMemory::AJAMemory()
{
}


AJAMemory::~AJAMemory()
{
}


void* 
AJAMemory::Allocate(size_t memorySize)
{
	if (memorySize == 0)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::Allocate  size is 0");
		return NULL;
	}

	void* pMemory = NULL;
	
	// allocate memory with no specific alignment
#if defined(AJA_WINDOWS)
	pMemory = malloc(memorySize);
#endif
#if defined(AJA_LINUX) || defined(AJA_MAC)
	pMemory = malloc(memorySize);
#endif

	if(pMemory == NULL)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::Allocate  allocation failed");
	}

	return pMemory;
}


void AJAMemory::Free(void* pMemory)
{
	if (pMemory == NULL)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::Free  memory address is NULL");
		return;
	}

	// free memory with no specific alignment
#if defined(AJA_WINDOWS)
	free(pMemory);
#endif
#if defined(AJA_LINUX) || defined(AJA_MAC)
	free(pMemory);
#endif
}


void* 
AJAMemory::AllocateAligned(size_t size, size_t alignment)
{
	if (size == 0)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::AllocateAligned  size is 0");
		return NULL;
	}

	void* pMemory = NULL;

	// allocate aligned memory
#if defined(AJA_WINDOWS)
	pMemory = _aligned_malloc(size, alignment);
#endif
#if defined(AJA_LINUX) || defined(AJA_MAC)
	if (posix_memalign(&pMemory, alignment, size))
		pMemory = NULL;
#endif

	if(pMemory == NULL)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::AllocateAligned  allocation failed size=%d alignment=%d", (int)size, (int)alignment);
	}

	return pMemory;
}


void 
AJAMemory::FreeAligned(void* pMemory)
{
	if (pMemory == NULL)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::FreeAligned  memory address is NULL");
		return;
	}

	if (pMemory == NULL)
	{
		return;
	}

	// free aligned memory
#if defined(AJA_WINDOWS)
	_aligned_free(pMemory);
#endif
#if defined(AJA_LINUX) || defined(AJA_MAC)
	free(pMemory);
#endif

}


void* 
AJAMemory::AllocateShared(size_t* pMemorySize, const char* pShareName)
{
	AJAAutoLock lock(&sSharedLock);
	
	// test parameters
	if (pMemorySize == NULL)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::AllocateShared  size is NULL");
		return NULL;
	}

	if (*pMemorySize == 0)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::AllocateShared  size is 0");
		return NULL;
	}
		
	if (pShareName == NULL) 
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::AllocateShared  share name is NULL");
		return NULL;
	}

	if (*pShareName == '\0') 
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::AllocateShared  share name is empty");
		return NULL;
	}

	// look for share with the same name
	size_t size = (*pMemorySize + AJA_PAGE_SIZE - 1) / AJA_PAGE_SIZE * AJA_PAGE_SIZE;

    std::string name;
#if defined(AJA_WINDOWS)
    name = "Global\\";
	name += pShareName;
#else //Mac and Linux
    // Docs say to start name with a slash
    name = "/";
    name += pShareName;
#endif

	std::list<SharedData>::iterator shareIter;

	for (shareIter = sSharedList.begin(); shareIter != sSharedList.end(); shareIter++)
	{
		// if share name match return this share
		if (name == shareIter->shareName)
		{
			// increment shared reference count
			shareIter->refCount++;

			// update memory size and return address
			*pMemorySize = shareIter->memorySize;
			return shareIter->pMemory;
		}
	}

	SharedData newData;

	// allocate new share
#if defined(AJA_WINDOWS)

	// Initialize a security descriptor.  
	PSECURITY_DESCRIPTOR  pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH); 
	BOOL rv = InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION);
	if (rv == FALSE)
	{
		DWORD err = GetLastError();
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::AllocateShared  InitializeSecurityDescriptor failed (err=%d)", err);
	}

	// Add the ACL to the security descriptor. 
	rv = SetSecurityDescriptorDacl(pSD, 
		TRUE,     // bDaclPresent flag   
		NULL,	  // this makes it completely open
		FALSE);   // not a default DACL 

	if (rv == FALSE)
	{
		DWORD err = GetLastError();
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::AllocateShared  SetSecurityDescriptorDacl failed (err=%d)", err);
	}

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof (SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;

	newData.fileMapHandle = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, (DWORD)size, name.c_str());
	LocalFree(pSD);
	if (newData.fileMapHandle == NULL)
	{
		DWORD err = GetLastError();
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::AllocateShared  CreateFileMapping failed (err=%d)", err);
		return NULL;
	}
	newData.pMemory = MapViewOfFile(newData.fileMapHandle, FILE_MAP_ALL_ACCESS, 0, 0, size);
	if (newData.pMemory == NULL)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::AllocateShared  MapViewOfFile failed");
		return NULL;
	}

	// In User Mode: Global\somename
	// In Kernel Mode: \BaseNamedObjects\somename
#else
    // Mac and Linux
	{
        newData.fileDescriptor = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
		if (newData.fileDescriptor < 0)
		{
			AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::AllocateShared  shm_open failed");
			return NULL;
		}
		// Despite 0666 above, write permissions for others was not set.  Force it.
		{
            std::string devFileName = "/dev/shm" + name;
            int rc = chmod(devFileName.c_str(), 0666);

			if (rc < 0)
			{
				#if !defined(AJA_MAC)
                    int saveErrno = errno;
                    syslog(LOG_ERR, "AJAMemory::AllocateShared chmod failed, err = %d\n", saveErrno);
				#endif
			}
		}

		// Creation is zero length. This call actually sets the size.
		int retVal = ftruncate(newData.fileDescriptor, size);

		if (0 != retVal)
		{
			AJA_REPORT(0, AJA_DebugSeverity_Error,
				"AJAMemory::AllocateShared  ftruncate failed");
		}

		newData.pMemory = mmap(NULL,
								size,
								PROT_READ | PROT_WRITE,
								MAP_SHARED,
								newData.fileDescriptor,
								0);
		if (newData.pMemory == NULL)
		{
			AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::AllocateShared  mmap failed");
			// Just because we failed, don't ruin it for others.  If we unlink, nobody will
			// be able to attach to the shared memory ever again.
			// shm_unlink(name.c_str());
			return NULL;
		}
	}
#endif  // AJA_LINUX || AJA_MAC

	// save details
	newData.shareName = name;
	newData.memorySize = size;
	newData.refCount = 1;

	// add to shared list
	sSharedList.push_back(newData);

	// update memory size and return address
	*pMemorySize = size;
	return newData.pMemory;
}


void 
AJAMemory::FreeShared(void* pMemory)
{
	AJAAutoLock lock(&sSharedLock);

	// look for memory at the same address
	std::list<SharedData>::iterator shareIter;
	for (shareIter = sSharedList.begin(); shareIter != sSharedList.end(); shareIter++)
	{
		if (pMemory == shareIter->pMemory)
		{
			shareIter->refCount--;
			if (shareIter->refCount <= 0)
			{
#if defined(AJA_WINDOWS)
				UnmapViewOfFile(shareIter->pMemory);
				CloseHandle(shareIter->fileMapHandle);
#endif
#if defined(AJA_LINUX) || defined(AJA_MAC)
				munmap(shareIter->pMemory, shareIter->memorySize);
				close(shareIter->fileDescriptor);
				// This is ugly. If we call shm_unlink, then the shared file name will
				// be removed from /dev/shm, and future calls to shm_open will create a
				// different memory share.  Will there be a problem with multiple calls
				// to shm_open with no intervening calls to shm_unlink?  Time will tell.
//				shm_unlink(shareIter->shareName.c_str());
#endif
				sSharedList.erase(shareIter);
			}
			return;
		}
	}

	AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAMemory::FreeShared  memory not found" /*, pMemory*/);
}
