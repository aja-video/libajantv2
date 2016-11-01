  /* 
    ntv2lindiskio.h:  linux specific video / audio disk i/o

    Linux specific implementation of the abstract base class
    CNtv2AbcFile.  CNtv2LinFile is not meant to be instantiated
    directly from a user app, rather the user app will instantiate an
    CNtv2File class object (which depending on the OS it runs under
    will inherit from CNtv2WinFile or CNtv2MacFile, etc.)

    Paul Maynard : 1st draft, 9-10-03
    Copyright (C) 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.

*/

#ifndef __NTV2_LIN_DISKIO__
#define __NTV2_LIN_DISKIO__

#include "ntv2abcdiskio.h"  //  Get AbstractBaseClass declarations

// Linux GUID struct hack.  into ajatypes.h with it? - jac
typedef struct {
	ULWord data_1;				// 32 bits
	unsigned short data_2;		// 16 bits
	unsigned short data_3;		// 16 bits
	UByte data_4[8];			// 64 bits
	
} GUID;

// Following GUID is for compatibility with InSync's SpeedRazor
static const GUID AjaXenaFileGuid = { 0x7218330e, 0xa7e3, 0x479d, { 0x86, 0x1f, 0x21, 0xed, 0xcc, 0x70, 0x6e, 0xee } };

// Linux specific implementation of the abstract base class
template<class FILE_HEADER>
class CNtv2LinFile : public CNtv2AbcFile<FILE_HEADER>
{
private:
	HANDLE              _hFile;
	CRITICAL_SECTION    _csOp;

    // Returns the sector size (4K) and sets the working directory -
    //  GetTempPath() uses the environment variables TMP, TEMP, or the 
    //  Windows directory
	LWord __get_sector_size ()
    {
		_lSectorSize = DEFAULT_SECTOR_SIZE;

		return _lSectorSize;
	};

    void CommonConstructorStuff ()
    {
        _hFile = NULL;
		InitializeCriticalSection(&_csOp);
		__get_sector_size();
    }
    
public:
    // Constructors
	CNtv2LinFile ()
    {
        CommonConstructorStuff();
	};

	CNtv2LinFile (string sFilePath, bool bReadAccess = false)
    {
        CommonConstructorStuff();
		OpenFile(sFilePath, bReadAccess);
	};

	CNtv2LinFile (const HANDLE hFile) {
		_hFile = hFile;
		CommonConstructorStuff();
	};

    // Destructor
	~CNtv2LinFile () {
		DeleteCriticalSection(&_csOp);
	};

    // Reference counting methods
	LWord AddRef () {
		return InterlockedIncrement((long*)&_lRef);
	};

	LWord ReleaseRef ()
    {
		if (InterlockedDecrement((long*)&_lRef) <= 0){
			delete this;
			return 0;
		}
		else{
		}
		return _lRef;
	};

    // File equivalency operator
	bool operator == (CNtv2LinFile & file) const
    {
		if (file._hFile == _hFile)
			return true;
		else
			return false;
	};

	bool IsOpened() const {
		return (_hFile && _hFile != INVALID_HANDLE_VALUE);
	};


    // GUID setters & getters
    void SetGUID (const GUID & guid)
    {
        memcpy(_XenaFileHeader.headerId, &guid, sizeof(GUID));
    }

    GUID & GetGUID (void)
    {
        return *((GUID*)&_XenaFileHeader.headerId);
    }

    ////////////////////////////////////////////////////////////////
    // File I/O methods
	ULWord ReadHeader ()
    {
		ULWord dwReadBytes;

		assert(_hFile && _hFile != INVALID_HANDLE_VALUE);
		assert((sizeof(FILE_HEADER) % DEFAULT_SECTOR_SIZE) == 0);

		EnterCriticalSection(&_csOp);
		__try{
			SetPosition(0, NULL);
			BOOL bRet = ::ReadFile(
				_hFile, 
                (LPVOID) &_XenaFileHeader,
				(DWORD)sizeof(FILE_HEADER), 
				(DWORD*)&dwReadBytes, 
				NULL);
            if (bRet && (dwReadBytes == sizeof(FILE_HEADER)))
                _bHeaderValid = true;
		}
		__finally{
			LeaveCriticalSection(&_csOp);
		}

		return dwReadBytes;
	};

	ULWord ReadBuffer (UByte *pBuff, const ULWord nBuffSize)
    {	
		ULWord dwReadBytes = 0;
 		assert(_hFile && _hFile != INVALID_HANDLE_VALUE);
		if(_lRef == 0)//if(!::InterlockedCompareExchange(&m_nRef, 0, 0))
			return 0;
		assert((nBuffSize % DEFAULT_SECTOR_SIZE) == 0);

		EnterCriticalSection(&_csOp);
		__try{
			::ReadFile(
				_hFile, 
				(LPVOID)pBuff, 
				(DWORD)nBuffSize, 
				(DWORD*)&dwReadBytes,
				NULL);
		}
		__finally{
			LeaveCriticalSection(&_csOp);
		}

		return dwReadBytes;
	};

    ULWord ReadAudioBuffer (UByte *pBuff, const ULWord nBuffSize)
    {	
		ULWord dwReadBytes = 0;
 		assert(_hFile && _hFile != INVALID_HANDLE_VALUE);
		if(_lRef == 0)//if(!::InterlockedCompareExchange(&m_nRef, 0, 0))
			return 0;

		EnterCriticalSection(&_csOp);
		__try{
			::ReadFile(
				_hFile, 
				(LPVOID)pBuff, 
				(DWORD)nBuffSize, 
				(DWORD*)&dwReadBytes,
				NULL);
		}
		__finally{
			LeaveCriticalSection(&_csOp);
		}

		return dwReadBytes;
	};

    ULWord ReadTcBuffer (UByte *pBuff, const ULWord nBuffSize)
    {
        return ReadAudioBuffer(pBuff, nBuffSize);
    }

	ULWord WriteHeader ()
    {
		ULWord dwWrittenBytes;
		
		assert(_hFile && _hFile != INVALID_HANDLE_VALUE);
		if(_lRef == 0)//if(!::InterlockedCompareExchange(&m_nRef, 0, 0))
			return 0;

		EnterCriticalSection(&_csOp);
		__try{
			SetPosition(0, NULL);
			::WriteFile(
				_hFile, 
				(LPCVOID)&_XenaFileHeader, 
				(DWORD)sizeof(FILE_HEADER), 
				(DWORD*)&dwWrittenBytes, 
				NULL);
		}
		__finally{
			LeaveCriticalSection(&_csOp);
		}

		return dwWrittenBytes;
	};

	ULWord WriteBuffer (UByte *pBuff, const ULWord nBuffSize)
    {
		ULWord dwWrittenBytes;
		
		assert(_hFile && _hFile != INVALID_HANDLE_VALUE);
		if(_lRef == 0)//if(!::InterlockedCompareExchange(&m_nRef, 0, 0))
			return 0;
		assert((nBuffSize % DEFAULT_SECTOR_SIZE) == 0);
		
		EnterCriticalSection(&_csOp);
		__try{
			::WriteFile(
				_hFile, 
				(LPCVOID)pBuff, 
				(DWORD)nBuffSize, 
				(DWORD*)&dwWrittenBytes,
				NULL);
		}
		__finally{
			LeaveCriticalSection(&_csOp);
		}

		return dwWrittenBytes;
	};

    ULWord WriteAudioBuffer (UByte *pBuff, const ULWord nBuffSize)
    {
		ULWord dwWrittenBytes;
		
		assert(_hFile && _hFile != INVALID_HANDLE_VALUE);
		if(_lRef == 0)//if(!::InterlockedCompareExchange(&m_nRef, 0, 0))
			return 0;
		
		EnterCriticalSection(&_csOp);
		__try{
			::WriteFile(
				_hFile, 
				(LPCVOID)pBuff, 
				(DWORD)nBuffSize, 
				(DWORD*)&dwWrittenBytes,
				NULL);
		}
		__finally{
			LeaveCriticalSection(&_csOp);
		}

		return dwWrittenBytes;
	};

    ULWord WriteTcBuffer (UByte *pBuff, const ULWord nBuffSize)
    {
        return WriteAudioBuffer (pBuff, nBuffSize);
    }

    /////////////////////////////////////////////////////////////////
    // File offset getting and setting methods
	ULWord GetPosition (PULWord pSeekHigh) const
    {
		LONG lSeekLow;
		
		assert(_hFile && _hFile != INVALID_HANDLE_VALUE);
		if (_lRef == 0)//if(!::InterlockedCompareExchange(&m_nRef, 0, 0))
			return 0;
		
		if (pSeekHigh)
			*pSeekHigh = 0;
		lSeekLow = ::SetFilePointer(_hFile, 0, (PLONG) pSeekHigh, FILE_CURRENT);
		return (ULWord) lSeekLow;
	};

	LWord64 GetPosition () const
    {
		LONG lSeekLow = 0, lSeekHigh = 0;
		
		assert(_hFile && _hFile != INVALID_HANDLE_VALUE);
		if(_lRef == 0)//if(!::InterlockedCompareExchange(&m_nRef, 0, 0))
			return 0;

		lSeekLow = ::SetFilePointer(_hFile, 0, &lSeekHigh, FILE_CURRENT);
		return (((LWord64)lSeekHigh << 32) | (LWord64)lSeekLow);
	};

	LWord64 SetPosition (LWord64 llSeek)
    {
		assert(_hFile && _hFile != INVALID_HANDLE_VALUE);
		if(_lRef == 0)//if(!::InterlockedCompareExchange(&m_nRef, 0, 0))
			return 0;

		EnterCriticalSection(&_csOp);
		__try{
			*((PLONG)&llSeek) = ::SetFilePointer(_hFile, (LONG)llSeek, (PLONG)&llSeek + 1, FILE_BEGIN);
		}
		__finally{
			LeaveCriticalSection(&_csOp);
		}
		return llSeek;
	};

	ULWord SetPosition (ULWord lSeekLow, PULWord pSeekHigh){
		assert(_hFile && _hFile != INVALID_HANDLE_VALUE);
		//if(!::InterlockedCompareExchange(&m_nRef, 0, 0))
		//	return 0;
        if(_lRef == 0)
            return 0;

		EnterCriticalSection(&_csOp);
		__try{
			lSeekLow = ::SetFilePointer(_hFile, lSeekLow, (PLONG) pSeekHigh, FILE_BEGIN);
		}
		__finally{
			LeaveCriticalSection(&_csOp);
		}
		return lSeekLow;
    };
    
	ULWord GoToFrame (ULWord frameNumber){
		assert(_hFile && _hFile != INVALID_HANDLE_VALUE);
		//if(!::InterlockedCompareExchange(&m_nRef, 0, 0))
		//	return 0;
		if(_lRef == 0)
			return 0;
		if ( frameNumber > _XenaFileHeader.numFrames)
			frameNumber = _XenaFileHeader.numFrames-1;
	
		ULWord ulVideoDiskSize = _XenaFileHeader.videoFrameSize;
		if (ulVideoDiskSize % DEFAULT_SECTOR_SIZE)
			ulVideoDiskSize = ((ulVideoDiskSize / DEFAULT_SECTOR_SIZE) + 1) *
			DEFAULT_SECTOR_SIZE;
		
		ULWord64 offset = (ULWord64)DEFAULT_SECTOR_SIZE + ((ULWord64)frameNumber*(ULWord64)ulVideoDiskSize);
		ULWord lSeekLow=(ULWord)offset;
		ULWord lSeekHigh = (ULWord)(offset>>32);
		EnterCriticalSection(&_csOp);
		__try{
			::SetFilePointer(_hFile, lSeekLow, (PLONG) &lSeekHigh, FILE_BEGIN);
		}
		__finally{
			LeaveCriticalSection(&_csOp);
		}
		return frameNumber;
	};

    ULWord64 GetDiskFreeSpaceBytes () const 
    {   
        ULARGE_INTEGER llFreeBytesAvailable;    // bytes available to caller
        ULARGE_INTEGER llTotalNumberOfBytes;    // bytes on disk
        ULARGE_INTEGER llTotalNumberOfFreeBytes;// free bytes on disk

        BOOL bRet = GetDiskFreeSpaceEx(_sWorkingDirectory.c_str(), &llFreeBytesAvailable,
            &llTotalNumberOfBytes, &llTotalNumberOfFreeBytes);
        ULWord64 llFreeBytes = 0;
        if (bRet)
            llFreeBytes = llFreeBytesAvailable.QuadPart;

        return llFreeBytes;
    };
    
    //////////////////////////////////////////////////////////////////
    // File open
	bool OpenFile (string sFilePath, bool bReadAccess = false)
    {
		CloseFile();

		_sFilePath = sFilePath;
        if (_sFilePath.empty())
        {
            if (!_sWorkingDirectory.empty())
            {
                _sFilePath = _sWorkingDirectory;  
                _sFilePath += ('_' + ::rand() + ".xvid");
            }
        }
        else
        {
            int iOff = _sFilePath.rfind('.');
            if (iOff)
            {
                int iLen = _sFilePath.length(); 
                iLen -= iOff;
                _sFilePath.erase(iOff, iLen);
            }
            
            _sFilePath += ".xvid";
        }
        
#ifdef _DEBUG
		OutputDebugString(_sFilePath.c_str());
		OutputDebugString(_T(": open file\n"));
#endif
		if(bReadAccess){
			_hFile = ::CreateFile(_sFilePath.c_str(),
				GENERIC_READ,
				FILE_SHARE_READ, 
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
				NULL);
		}
		else{
			_hFile = ::CreateFile(_sFilePath.c_str(),
				GENERIC_WRITE,
				0, NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
				NULL);

            SetGUID(AjaXenaFileGuid);   // For InSync's SpeedRazor
		}

		if(_hFile == INVALID_HANDLE_VALUE)
			return false;

		::GetFileSizeEx(_hFile, (PLARGE_INTEGER) &_llSize);

        SetWorkingPath (_sFilePath);
 
		return true;
	};

     //////////////////////////////////////////////////////////////////
    // Audio File open
	bool OpenAudioFile (string sFilePath, bool bReadAccess = false)
    {
		CloseFile();

		_sFilePath = sFilePath;
        if (_sFilePath.empty())
        {
            if (!_sWorkingDirectory.empty())
            {
                _sFilePath = _sWorkingDirectory;  
                _sFilePath += ('_' + ::rand() + ".xaud");
            }
		}
        else
        {
            int iOff = _sFilePath.rfind('.');
            if (iOff)
            {
                int iLen = _sFilePath.length(); 
                iLen -= iOff;
                _sFilePath.erase(iOff, iLen);
            }
            
            _sFilePath += ".xaud";
        }
        
#ifdef _DEBUG
		OutputDebugString(_sFilePath.c_str());
		OutputDebugString(_T(": open file\n"));
#endif
		if(bReadAccess){
			_hFile = ::CreateFile(_sFilePath.c_str(),
				GENERIC_READ,
				FILE_SHARE_READ, 
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL);
		}
		else{
			_hFile = ::CreateFile(_sFilePath.c_str(),
				GENERIC_WRITE,
				0, NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL);
		}

		if(_hFile == INVALID_HANDLE_VALUE)
			return false;

		::GetFileSizeEx(_hFile, (PLARGE_INTEGER) &_llSize);
 
        SetWorkingPath (_sFilePath);
        
		return true;
	};

    //////////////////////////////////////////////////////////////////
    // RP188 File open
	bool OpenTcFile (string sFilePath, bool bReadAccess = false)
    {
        CloseFile();

		_sFilePath = sFilePath;
        if (_sFilePath.empty())
        {
            if (!_sWorkingDirectory.empty())
            {
                _sFilePath = _sWorkingDirectory;  
                _sFilePath += ('_' + ::rand() + ".xtc");
            }
		}
        // Insure that extension is ".Xtc"
        else
        {
            int iOff = _sFilePath.rfind('.');
            if (iOff)
            {
                int iLen = _sFilePath.length(); 
                iLen -= iOff;
                _sFilePath.erase(iOff, iLen);
            }
            
            _sFilePath += ".xtc";
        }
        
#ifdef _DEBUG
		OutputDebugString(_sFilePath.c_str());
		OutputDebugString(_T(": open file\n"));
#endif
		if(bReadAccess){
			_hFile = ::CreateFile(_sFilePath.c_str(),
				GENERIC_READ,
				FILE_SHARE_READ, 
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL);
		}
		else{
			_hFile = ::CreateFile(_sFilePath.c_str(),
				GENERIC_WRITE,
				0, NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL);
		}

		if(_hFile == INVALID_HANDLE_VALUE)
			return false;

		::GetFileSizeEx(_hFile, (PLARGE_INTEGER) &_llSize);
 
        SetWorkingPath (_sFilePath);
        
		return true;
	};

    ////////////////////////////////////////////////////////////////
    // File Close
	void CloseFile()
    {
		if (_hFile && _hFile != INVALID_HANDLE_VALUE)
        {
			CloseHandle(_hFile);
            _hFile = 0;

#ifdef _DEBUG
			OutputDebugString(_sFilePath.c_str());
			OutputDebugString(_T(": close file\n"));
#endif
		}

		return;
	};

};

#endif  //   __NTV2_LIN_DISKIO__
