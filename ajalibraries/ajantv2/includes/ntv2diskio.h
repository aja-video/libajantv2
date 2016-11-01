/**
	@file		ntv2diskio.h
	@brief		Declares and implements the CXenaFile class.
	@copyright	Copyright 2004-2016 AJA Video Systems, Inc. All rights reserved.
**/
#ifndef __NTV2__DISKIO__
#define __NTV2__DISKIO__


#if defined (MSWindows)
	#include "ntv2windiskio.h"
#endif
#if defined(AJALinux)
	#include "ntv2lindiskio.h"
#endif

template<class FILE_HEADER> class CXenaFile
#if defined(MSWindows)
	:	public CNtv2WinFile <FILE_HEADER>
#endif
#if defined(AJALinux)
	:	public CNtv2LinFile <FILE_HEADER>
#endif
{
private:
    void CommonConstructorStuff ()
    {
        memset ((void *) &_XenaFileHeader, 0, sizeof(_XenaFileHeader));
        _bHeaderValid = false;
        _XenaFileHeader.headerSize	= sizeof(_XenaFileHeader);
    }

public:
	CXenaFile()
    {
        CommonConstructorStuff();
	};

	CXenaFile(string sFilePath, bool bReadAccess = false)
    {
        CommonConstructorStuff();
		OpenFile(sFilePath, bReadAccess);
	};

	~CXenaFile(){
		CloseFile();
	};

    /////////////////////////////////////////////////////////////////////
    // Getters
    inline bool HasVideo (void) const
    {
        if (_XenaFileHeader.withVideo)
            return true;
        else
            return false;
    }

    inline bool HasAudio (void) const
    {
        if (_XenaFileHeader.withAudio)
            return true;
        else
            return false;
    }

    inline bool HasRP188 (void) const
    {
        if (_XenaFileHeader.withRP188)
            return true;
        else
            return false;
    }
    
    inline NTV2VideoFormat GetVideoFormat (void) const
    {
        return _XenaFileHeader.videoFormat;
    }

    inline NTV2FrameBufferFormat GetFrameBufferFormat (void) const
    {
        return _XenaFileHeader.frameBufferFormat;
    }

    inline ULWord GetNumFrames (void) const
    {
        return _XenaFileHeader.numFrames;
    }

    inline ULWord GetVideoFrameSize (void) const
    {
        return _XenaFileHeader.videoFrameSize;
    }

    inline ULWord GetVideoDiskSize (void) const
    {
        ULWord ulVideoDiskSize = _XenaFileHeader.videoFrameSize;
        if (ulVideoDiskSize % DEFAULT_SECTOR_SIZE)
            ulVideoDiskSize = ((ulVideoDiskSize / DEFAULT_SECTOR_SIZE) + 1) *
                DEFAULT_SECTOR_SIZE;
        
        return ulVideoDiskSize;
    }

    inline ULWord GetVideoRate (void) const
    {
        return _XenaFileHeader.videoRate;
    }

    inline ULWord GetVideoScale (void) const
    {
        return _XenaFileHeader.videoScale;
    }

    // audio
    inline ULWord GetAudioSampleRate (void) const
    {
        return _XenaFileHeader.audioSampleRate;
    }

    inline NTV2AudioRate GetAudioRate (void) const
    {
        return _XenaFileHeader.audioRate;
    }
    
    inline ULWord64 GetAudioNumSamples (void) const
    {
        return _XenaFileHeader.audioNumSamples;
    }

    inline ULWord GetAudioSampleSize (void) const
    {
        return _XenaFileHeader.audioSampleSize;
    }

    inline ULWord GetAudioChannels (void) const
    {
        return _XenaFileHeader.audioChannels;
    }
    
    inline UByte GetAudioChanMask (void) const
    {
        return _XenaFileHeader.audioChanMask;
    }

    inline ULWord GetAudioFrameSize (void) const
    {
        return _XenaFileHeader.audioFrameSize;
    }
    
    inline ULWord GetAudioStereoMap1 (void) const
    {
        return _XenaFileHeader.audioStereoMap1;
    }

    inline ULWord GetAudioStereoMap2 (void) const
    {
        return _XenaFileHeader.audioStereoMap2;
    }

    bool GetAudioFilePath (string & sOutAudFilePath)
    {
        bool bRet = true;
        if (_XenaFileHeader.withAudio)
            sOutAudFilePath = _sFilePath;
        else if (_XenaFileHeader.withVideoAudio)
        {
            sOutAudFilePath = _sFilePath;
            if (sOutAudFilePath.rfind("XVid") < sOutAudFilePath.length())
                sOutAudFilePath.replace(sOutAudFilePath.rfind("XVid"), 4, "XAud");
            else
                bRet = false;
        }
        else
            bRet = false;

        return bRet;
    }

    bool GetVideoFilePath (string & sOutVidFilePath)
    {
        bool bRet = true;
        if (_XenaFileHeader.withVideo)
            sOutVidFilePath = _sFilePath;
        else if (_XenaFileHeader.withVideoAudio)
        {
            sOutVidFilePath = _sFilePath;
            if (sOutVidFilePath.rfind("XAud") < sOutVidFilePath.length())
                sOutVidFilePath.replace(sOutVidFilePath.rfind("XAud"), 4, "XVid");
            else
                bRet = false;
        }
        else
            bRet = false;

        return bRet;
    }

    bool GetTcFilePath (string & sOutTcFilePath)
    {
        bool bRet = true;
        if (_XenaFileHeader.withRP188 == 0)
        {
            bRet = false;
        }
        else
        {
            sOutTcFilePath = _sFilePath;
            if (sOutTcFilePath.rfind("XVid") < sOutTcFilePath.length())
                sOutTcFilePath.replace(sOutTcFilePath.rfind("XVid"), 4, "Xtc");
            else if (sOutTcFilePath.rfind("XAud") < sOutTcFilePath.length())
                sOutTcFilePath.replace(sOutTcFilePath.rfind("XAud"), 4, "Xtc");
            else
                bRet = false;
        }

        return bRet;
    }
    
    
    /////////////////////////////////////////////////////////////////////
    // Setters

    // Video
    inline void SetWithVideo (void)
    {
        _XenaFileHeader.withVideo = 1;
    }

    inline void SetVideoFormat (const NTV2VideoFormat vf)
    {
        _XenaFileHeader.videoFormat = vf;
    }
    
    inline void SetFrameBufferFormat (const NTV2FrameBufferFormat fbf) 
    {
         _XenaFileHeader.frameBufferFormat = fbf;
    }

    inline void SetVideoScale (const ULWord vs) 
    {
        _XenaFileHeader.videoScale = vs;
    }

    inline void SetVideoRate (const ULWord vr) 
    {
        _XenaFileHeader.videoRate = vr;
    }

    inline void SetVideoFrameSize (const ULWord fs) 
    {
        _XenaFileHeader.videoFrameSize = fs;
    }

    inline void SetVideoWriteSize (const ULWord ws) 
    {
        _XenaFileHeader.videoWriteSize = ws;
    }

    inline void SetVideoNumFrames (const ULWord vnf)
    {
        _XenaFileHeader.numFrames = vnf;
    }

    // Audio
    inline void SetWithAudio (void)
    {
        _XenaFileHeader.withAudio = 1;
    }

    inline void SetAudioRate (const NTV2AudioRate ar)
    {
        _XenaFileHeader.audioRate = ar;
    }

    inline void SetAudioSampleRate (const ULWord asr)
    {
        _XenaFileHeader.audioSampleRate = asr;
    }

    inline void SetAudioChannels (const UByte ac)
    {
        _XenaFileHeader.audioChannels = ac;
    }

    inline void SetAudioFrameSize (const ULWord afs)
    {
        _XenaFileHeader.audioFrameSize = afs;
    }

    inline void SetAudioSampleSize (const ULWord as)
    {
        _XenaFileHeader.audioSampleSize = as;
    }

    inline void SetAudioWriteSize (const ULWord aws)
    {
        _XenaFileHeader.audioWriteSize = aws;
    }
    
    inline void SetAudioNumChannels (const UByte nc)
    {
        _XenaFileHeader.audioChannels = nc;
    }

    inline void SetAudioChanMask (const UByte cm)
    {
        _XenaFileHeader.audioChanMask = cm;
    }

    inline void SetAudioStereoMap1 (const ULWord asm1)
    {
        _XenaFileHeader.audioStereoMap1 = asm1;
    }

    inline void SetAudioStereoMap2 (const ULWord asm2)
    {
        _XenaFileHeader.audioStereoMap2 = asm2;
    }

    inline void SetWithVideoAudio (void)
    {
        _XenaFileHeader.withVideoAudio = 1;
    }
    
    inline void SetAudioNumSamples (const ULWord64 ans) 
    {
        _XenaFileHeader.audioNumSamples = ans;
    }

    // RP188
    inline void SetWithRP188 (void)
    {
        _XenaFileHeader.withRP188 = 1;
    }

};	//	CXenaFile

#endif  //   __NTV2__DISKIO__
