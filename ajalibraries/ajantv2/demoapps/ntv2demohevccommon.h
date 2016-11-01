/**
	@file		ntv2demohevccommon.h
	@brief		This file contains some structures, constants, classes and functions that are used in some of the hevc demo applications.
				There is nothing magical about anything in this file. In your applications you may use a different
				number of circular buffers, or store different data in the AVDataBuffer. What's listed below
				are simply values that work well with the demos.
	@copyright	Copyright (C) 2016 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2DEMOHEVCCOMMON_H
#define _NTV2DEMOHEVCCOMMON_H

#include "ntv2rp188.h"
#include "ajastuff/common/videotypes.h"
#include "ajastuff/common/circularbuffer.h"

#include "ntv2m31enums.h"
#include "ntv2m31.h"

/**
	@brief	This structure encapsulates the video and audio buffers used by the HEVC demo applications. The demo 
            programs that employ producer/consumer threads use a fixed number of these buffers.
**/
typedef struct
{
    uint32_t *		pVideoBuffer;			///< @brief	Pointer to host video buffer
    uint32_t		videoBufferSize;		///< @brief	Size of host video buffer (bytes)
    uint32_t		videoDataSize;			///< @brief	Size of video data (bytes)
    uint32_t		videoDataSize2;			///< @brief	Size of field 2 video data (bytes)
    uint32_t *      pInfoBuffer;            ///< @brief Picture information (raw) or encode information (hevc)
    uint32_t        infoBufferSize;         ///< @brief Size of the host information buffer (bytes)
    uint32_t        infoDataSize;           ///< @brief Size of the information data (bytes)
    uint32_t        infoDataSize2;          ///< @brief Size of the field 2 information data (bytes)
    uint32_t *		pAudioBuffer;			///< @brief	Pointer to host audio buffer
    uint32_t		audioBufferSize;		///< @brief	Size of host audio buffer (bytes)
    uint32_t		audioDataSize;			///< @brief	Size of audio data (bytes)
    uint32_t        timeCodeDBB;            ///< @brief Time code data dbb
    uint32_t        timeCodeLow;            ///< @brief Time code data low
    uint32_t        timeCodeHigh;           ///< @brief Time code data high
	int64_t			frameTime;				///< @brief Capture time stamp
    bool			lastFrame;				///< @brief Indicates last captured frame
} AVHevcDataBuffer;


typedef struct
{
    ULWord          framesProcessed;
    ULWord          framesDropped;
    ULWord          bufferLevel;
} AVHevcStatus;


class CNTV2DemoHevcCommon
{
    //	Public Instance Methods
	public:
                                    CNTV2DemoHevcCommon ();
                                    ~CNTV2DemoHevcCommon ();
    
        AJA_PixelFormat             GetAJAPixelFormat(NTV2FrameBufferFormat pixelFormat);
        AJA_FrameRate               GetAJAFrameRate(NTV2FrameRate frameRate);

        AJAStatus                   CreateHevcFile(const char* pFileName, uint32_t maxFrames);
        void                        CloseHevcFile(void);
        void                        WriteHevcData(void* pBuffer, uint32_t bufferSize);
    
        AJAStatus                   CreateEncFile(const char* pFileName, uint32_t maxFrames);
        void                        CloseEncFile(void);
        void                        WriteEncData(void* pBuffer, uint32_t bufferSize);
    
        AJAStatus                   CreateAiffFile(const char* pFileName, uint32_t numChannels, uint32_t maxFrames, uint32_t bufferSize);
        void                        CloseAiffFile(void);
        void                        WriteAiffHeader(void);
        void                        WriteAiffData(void* pBuffer, uint32_t numChannels, uint32_t numSamples);

        AJAStatus                   CreateRawFile(const char* pFileName, uint32_t maxFrames);
        void                        CloseRawFile(void);
        void                        WriteRawData(void* pBuffer, uint32_t bufferSize);
    
        AJAStatus                   OpenYuv420File(const char* pFileName, const uint32_t width, const uint32_t height);
        void                        CloseYuv420File(void);
        AJAStatus                   ReadYuv420Frame(void* pBuffer, uint32_t numFrame);
        AJAStatus                   ConvertYuv420FrameToNV12(void* pSrcBuffer, void* pDstBuffer, uint32_t bufferSize);
        uint64_t                    YuvFileSize() { return mYuvFileSize; }
        uint32_t                    YuvNumFrames() { return mYuvNumTotalFrames; }
        uint32_t                    YuvWidth() { return mYuvFrameWidth; }
        uint32_t                    YuvHeight() { return mYuvFrameHeight; }

        uint32_t                    AlignDataBuffer(void* pBuffer, uint32_t bufferSize, uint32_t dataSize, uint32_t alignBytes, uint8_t fill);
        AJAStatus                   DetermineInputFormat(NTV2VideoFormat sdiFormat, bool quad, NTV2VideoFormat& videoFormat);

        AJAStatus                   SetupHEVC (CNTV2m31 * pM31, M31VideoPreset preset, M31Channel encodeChannel, bool multiStream, bool withInfo);

    //	Private Member Data
    private:
        FILE *						mHevcFd;                                    			/// Descriptor for HEVC output file
        FILE *                      mEncFd;                                                 /// Descriptor for Encoded data file
        FILE *						mAiffFd;                                    			/// Descriptor for AIFF output file
        FILE *						mYuvFd;                                                 /// Descriptor for YUV inupt file
        FILE *						mRawFd;                                                 /// Descriptor for YUV inupt file
    
        uint32_t                    mHevcFileFrameCount;                                    /// HEVC file number of frames written
        uint32_t                    mMaxHevcFrames;                                         /// Maximum number of frames to write to HEVC file

        uint32_t                    mEncFileFrameCount;                                     /// Encoded data file number of frames written
        uint32_t                    mMaxEncFrames;                                          /// Maximum number of frames to write to encoded data file

        uint32_t                    mAiffFileFrameCount;                                    /// Audio file number of frames written
        uint32_t                    mMaxAiffFrames;                                         /// Maximum number of frames to write to AIFF file
    
        uint32_t                    mAiffTotalSize;                                         /// AIFF file total size (bytes)
        uint32_t                    mAiffNumSamples;                                        /// AIFF file number of samples
        uint32_t                    mAiffNumChannels;                                       /// AIFF file number of channels
        uint8_t*                    mAiffWriteBuffer;                                       /// AIFF audio write buffer
    
        uint64_t                    mYuvFileSize;
        uint32_t                    mYuvFrameWidth;
        uint32_t                    mYuvFrameHeight;
        uint32_t                    mYuvNumTotalFrames;
        uint32_t                    mYuvFrameSize;

        uint32_t                    mRawFileFrameCount;                                    /// Raw file number of frames written
        uint32_t                    mMaxRawFrames;                                         /// Maximum number of frames to write to raw file

};	//	CNTV2DemoCommon


#endif	//	_NTV2DEMOHEVCCOMMON_H
