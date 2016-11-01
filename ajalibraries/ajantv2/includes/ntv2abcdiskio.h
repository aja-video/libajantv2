/*
    ntv2abcdiskio.h: virtual methods for multi-os video / audio disk i/o
	Copyright (C) 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.

    Abstract base class for x-platform disk i/o.
    Based on work from Nikolay Balov(nikolai.balov@atia.com)	20/1/03.

    Paul Maynard : 1st draft, 9-10-03

*/

#ifndef __NTV2_ABC_DISKIO_FILE__
#define __NTV2_ABC_DISKIO_FILE__


#include "ntv2enums.h"                  // from Aja "\includes" directory
#include "ntv2card.h"                   // from Aja "\classes" directory

#include "videodefines.h"               // from Aja "\includes" directory
#include "audiodefines.h"

const int DEFAULT_SECTOR_SIZE = 0x1000;  // 4KB
const int GUID_SIZE = 16;                // GUIDs are 16 bytes long



// XENA_FILE_INFO specific file format
typedef struct {
	UByte					headerId[GUID_SIZE];
	ULWord					headerSize;
    NTV2VideoFormat			videoFormat;
    NTV2FrameBufferFormat	frameBufferFormat;
	ULWord					videoScale;
	ULWord					videoRate;
	ULWord					videoFrameSize;
	ULWord					videoWriteSize;
	ULWord					withAudio;
	NTV2AudioRate			audioRate;
	ULWord					audioSampleRate;
	ULWord					audioSampleSize;
	ULWord					audioFrameSize; 
	ULWord					audioWriteSize;
	ULWord					audioStereoMap1;
	ULWord					audioStereoMap2;
    ULWord					numFrames;
	ULWord64				timecodeOffset; // if 0 no timecode presents
	ULWord64				timecodeStart;
    UByte                   audioChannels;  // PFM
    UByte                   audioChanMask;  // PFM
    ULWord                  withVideo;      // PFM
    ULWord64                audioNumSamples;// PFM
    UByte                   withRP188;      // PFM
    UByte                   withVideoAudio; // PFM
    UByte					reserved[DEFAULT_SECTOR_SIZE - (
  
        GUID_SIZE +
		sizeof(ULWord) +
		sizeof(NTV2VideoFormat) + 
		sizeof(NTV2FrameBufferFormat) + 
		6 * sizeof(ULWord) + 
		sizeof(NTV2AudioRate) + 
		7 * sizeof(ULWord) +
		2 * sizeof(ULWord64) +
        2 * sizeof(ULWord64) +
        4 * sizeof(UByte))
	];

// Only Windows will use GUIDS, but these have to be declared / defined here
// so they will be part of the NTV2_FILE_INFO structure
#ifdef MSWindows

	// be careful using this code
	void _guid_encode()
    {	
		int n, size, *pg, *ph;
		
		size = headerSize - sizeof(GUID);
		n = 0;
		pg = (int*)&headerId;
		ph = (int*)&headerSize;
		while(n < size){
			// suppose sizeof(GUID) == 16
			ph[0] ^= pg[0];
			ph[1] ^= pg[1];
			ph[2] ^= pg[2];
			ph[3] ^= pg[3];
			ph += sizeof(GUID)/sizeof(int);
			n += sizeof(GUID);
		}
	};

	void _guid_decode()
    {	
		int n, size, *pg, *ph;
		
		n = 0;
		pg = (int*)&headerId;
		ph = (int*)&headerSize;

		ph[0] ^= pg[0];
		ph[1] ^= pg[1];
		ph[2] ^= pg[2];
		ph[3] ^= pg[3];
		ph += sizeof(GUID)/sizeof(int);
		n += sizeof(GUID);

		// check consistency
		if(headerSize > sizeof(XENA_FILE_INFO)){
			memset(this, 0, sizeof(XENA_FILE_INFO));
#  ifdef _DEBUG
			OutputDebugString(_T("_guid_decode: File corrupt\n"));
#  endif
			return;
		}

		size = headerSize - sizeof(GUID);
		while(n < size){
			// suppose sizeof(GUID) == 16
			ph[0] ^= pg[0];
			ph[1] ^= pg[1];
			ph[2] ^= pg[2];
			ph[3] ^= pg[3];
			ph += sizeof(GUID)/sizeof(int);
			n += sizeof(GUID);
		}
	};

#endif  // MSWindows

} XENA_FILE_INFO;


///////////////////////////////////////////////////////////////////
// Declaration of the abstract base class
template<class FILE_HEADER>
class CNtv2AbcFile
{
//protected:
public:
	volatile LWord _lRef;           // reference count for open file
    string  _sWorkingDirectory;     // read from environment variables or set by user
    string  _sFilePath;             // user specified filepath of current file
	LWord   _lSectorSize;           // for now hardwired to 4KB (4096 bytes)
	LWord64 _llSize;                // size of the opened file

    XENA_FILE_INFO _XenaFileHeader;
    bool           _bHeaderValid;

    // Returns the sectorSize and sets the WorkingDirectory
	virtual LWord __get_sector_size() = 0;

public:
    // Constructor
	CNtv2AbcFile() 
    {
        _sFilePath = "";
        _sWorkingDirectory = "";
        _lRef = 0;
        _lSectorSize = 0;
        _llSize = 0;
	};

    // Destructor - virtual to insure proper destruction of children
    virtual ~CNtv2AbcFile () {}

    // Pure virtual methods meant to be implemented by the OS specific classes
    virtual bool OpenFile (string sFilePath, bool bReadAccess = false) = 0;

	virtual LWord AddRef () = 0;

	virtual LWord ReleaseRef () = 0;

	virtual bool IsOpened () const = 0;

    virtual ULWord ReadHeader (void) = 0;

	virtual ULWord ReadBuffer (UByte *pBuff, const ULWord nBuffSize) = 0;
 
	virtual ULWord WriteHeader (void) = 0;

	virtual ULWord WriteBuffer (UByte *pBuff, const ULWord nBuffSize) = 0;

	virtual ULWord GetPosition (PULWord pSeekHigh) const = 0;
 
	virtual LWord64 GetPosition () const = 0;

	virtual LWord64 SetPosition (LWord64 llSeek) = 0;

	virtual ULWord SetPosition (ULWord lSeekLow, PULWord pSeekHigh) = 0;

    virtual ULWord64 GetDiskFreeSpaceBytes() const = 0;

	virtual void CloseFile() = 0;

    // Get/Set(s) that can be done in the ABC because the members are 
    //  local
    const string & GetWorkingPath () const {
		return _sWorkingDirectory;
	};

	const string & GetFilePath () const {
		return _sFilePath;
	};

	const LWord64 GetFileSize () const {
		return _llSize;
	};

	void SetWorkingPath (string &sPath) {
        _sWorkingDirectory = sPath;
        int iOff = (int)_sWorkingDirectory.rfind("\\");
        if (iOff)
            _sWorkingDirectory.erase(iOff + 1, _sWorkingDirectory.length());
        else
            _sWorkingDirectory += "\\";

	};
};



/* let write size be 4KB aligned */
#define WRITE_SIZE(size) (((size) + (UINT)0x00000fff) & 0xfffff000)

/* let write size be 4KB aligned */
#define VIDEO_WRITE_SIZE(size) (((size) + (UINT)0x00000fff) & 0xfffff000)

/* let write size be 4KB aligned */
#define AUDIO_WRITE_SIZE(size) ( \
	((size) + (size)/4/* quater size tolerance */ + 2 * sizeof(LWord)/* place for m_audioReadSize & m_audioStartSample */ + \
	NTV2_NUMAUDIO_CHANNELS * sizeof(LWord) + \
	/* for proper alignment */ (LWord)0x00000fff) & 0xfffff000)


#endif // __NTV2_FILE__
