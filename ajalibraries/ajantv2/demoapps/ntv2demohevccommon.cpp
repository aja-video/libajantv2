/**
	@file		ntv2demohevccommon.cpp
	@brief		Common implementation code used by many of the demo applications.
	@copyright	Copyright (C) 2013-2015 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2demohevccommon.h"
#include "ntv2devicescanner.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include <assert.h>
#include <map>
#include <sys/stat.h>

using namespace std;

CNTV2DemoHevcCommon::CNTV2DemoHevcCommon ()
:   mHevcFd                 (NULL),
    mEncFd                  (NULL),
    mAiffFd                 (NULL),
    mYuvFd                  (NULL),
    mRawFd                  (NULL),
    mHevcFileFrameCount     (0),
    mMaxHevcFrames          (0),
    mEncFileFrameCount      (0),
    mMaxEncFrames           (0),
    mAiffFileFrameCount     (0),
    mMaxAiffFrames          (0),
    mAiffTotalSize          (0),
    mAiffNumSamples         (0),
    mAiffNumChannels        (0),
    mAiffWriteBuffer        (0),
    mYuvFileSize            (0),
    mYuvFrameWidth          (0),
    mYuvFrameHeight         (0),
    mYuvNumTotalFrames      (0),
    mYuvFrameSize           (0),
    mRawFileFrameCount      (0),
    mMaxRawFrames           (0)

{
}


CNTV2DemoHevcCommon::~CNTV2DemoHevcCommon ()
{
    // Just in case
    CloseHevcFile();
    CloseEncFile();
    CloseAiffFile();
    CloseYuv420File();
    
}

AJA_PixelFormat CNTV2DemoHevcCommon::GetAJAPixelFormat(NTV2FrameBufferFormat pixelFormat)
{
    switch (pixelFormat)
    {
        case NTV2_FBF_8BIT_YCBCR: return AJA_PixelFormat_YCbCr8;
        case NTV2_FBF_10BIT_YCBCR: return AJA_PixelFormat_YCbCr10;
        case NTV2_FBF_10BIT_YCBCR_420PL: return AJA_PixelFormat_YCBCR10_420PL;
        case NTV2_FBF_10BIT_YCBCR_422PL: return AJA_PixelFormat_YCBCR10_422PL;
        case NTV2_FBF_8BIT_YCBCR_420PL: return AJA_PixelFormat_YCBCR8_420PL;
        case NTV2_FBF_8BIT_YCBCR_422PL: return AJA_PixelFormat_YCBCR8_422PL;
        default: break;
    }
    
    return AJA_PixelFormat_Unknown;
}


AJA_FrameRate CNTV2DemoHevcCommon::GetAJAFrameRate(NTV2FrameRate frameRate)
{
    switch (frameRate)
    {
        case NTV2_FRAMERATE_2398: return AJA_FrameRate_2398;
        case NTV2_FRAMERATE_2400: return AJA_FrameRate_2400;
        case NTV2_FRAMERATE_2500: return AJA_FrameRate_2500;
        case NTV2_FRAMERATE_2997: return AJA_FrameRate_2997;
        case NTV2_FRAMERATE_3000: return AJA_FrameRate_3000;
        case NTV2_FRAMERATE_4795: return AJA_FrameRate_4795;
        case NTV2_FRAMERATE_4800: return AJA_FrameRate_4800;
        case NTV2_FRAMERATE_5000: return AJA_FrameRate_5000;
        case NTV2_FRAMERATE_5994: return AJA_FrameRate_5994;
        case NTV2_FRAMERATE_6000: return AJA_FrameRate_6000;
        default: break;
    }
    
    return AJA_FrameRate_Unknown;
}


AJAStatus CNTV2DemoHevcCommon::CreateHevcFile(const char* pFileName, uint32_t maxFrames)
{
    //  Close file if already open
    if (mHevcFd != NULL)
    {
        CloseHevcFile();
    }
    
    //  Open binary output
    mHevcFd = fopen(pFileName, "wb");
    if (mHevcFd == NULL)
    {
        return AJA_STATUS_OPEN;
    }
    mMaxHevcFrames = maxFrames;
    
    return AJA_STATUS_SUCCESS;
}


void CNTV2DemoHevcCommon::CloseHevcFile(void)
{
    if (mHevcFd != NULL)
    {
        fclose(mHevcFd);
        mHevcFd = NULL;
        mMaxHevcFrames = 0;
    }
}


void CNTV2DemoHevcCommon::WriteHevcData(void* pBuffer, uint32_t bufferSize)
{
    if ((mHevcFd == NULL) ||
        (pBuffer == NULL) ||
        (bufferSize == 0) ||
        (mMaxHevcFrames == 0)) return;
    
    if((mMaxHevcFrames != 0xffffffff) && (mHevcFileFrameCount > mMaxHevcFrames))
    {
        fseek (mHevcFd, 0, SEEK_SET);
        mHevcFileFrameCount = 0;
    }
    
    fwrite(pBuffer, 1, bufferSize, mHevcFd);
    mHevcFileFrameCount++;
}


AJAStatus CNTV2DemoHevcCommon::CreateEncFile(const char* pFileName, uint32_t maxFrames)
{
    //  Close file if already open
    if (mEncFd != NULL)
    {
        CloseEncFile();
    }
    
    //  Open text output
    mEncFd = fopen(pFileName, "w");
    if (mEncFd == NULL)
    {
        return AJA_STATUS_OPEN;
    }
    mMaxEncFrames = maxFrames;
    
    return AJA_STATUS_SUCCESS;
}


void CNTV2DemoHevcCommon::CloseEncFile(void)
{
    if (mEncFd != NULL)
    {
        fclose(mEncFd);
        mEncFd = NULL;
        mMaxEncFrames = 0;
    }
}


void CNTV2DemoHevcCommon::WriteEncData(void* pBuffer, uint32_t bufferSize)
{
    if ((mEncFd == NULL) ||
        (pBuffer == NULL) ||
        (bufferSize < sizeof(HevcEncodedData)) ||
        (mMaxEncFrames == 0)) return;
    
    if((mMaxEncFrames != 0xffffffff) && (mEncFileFrameCount > mMaxEncFrames))
    {
        fseek (mEncFd, 0, SEEK_SET);
        mEncFileFrameCount = 0;
    }
    
    HevcEncodedData* pEncData = (HevcEncodedData*)pBuffer;
    
    const char* pPicType = "Unknown";
    switch(pEncData->pictureType)
    {
        case 0: pPicType = "I-frame"; break;
        case 1: pPicType = "P-frame"; break;
        case 2: pPicType = "B-frame"; break;
        default: break;
    }
    
    long long unsigned int offset = (((uint64_t)pEncData->esOffsetHigh)<<32) + (uint64_t)pEncData->esOffsetLow;
    long long int pts = ((((uint64_t)pEncData->ptsValueHigh)<<32) + (uint64_t)pEncData->ptsValueLow);
    long long int dts = ((((uint64_t)pEncData->dtsValueHigh)<<32) + (uint64_t)pEncData->dtsValueLow);
    
    fprintf(mEncFd, "Serial number: %d   Picture type: %s   Last frame: %s\n",
            pEncData->serialNumber, pPicType, pEncData->esLastFrame?"Y":"N");
    fprintf(mEncFd, "Frame offset: 0x%0llx  Frame Size: %d\n",
            offset, pEncData->esSize);
    fprintf(mEncFd, "PTS: %lld   DTS: %lld\n",
            pts, dts);
    fprintf(mEncFd, "Temporal ID: %d   NAL offset: 0x%x\n",
            pEncData->temporalId, pEncData->nalOffset);
    fprintf(mEncFd, "CPB data size: %d   Additional data: %d\n\n",
            pEncData->cpbValue, pEncData->numAdditionalData);
    
    mEncFileFrameCount++;
}


AJAStatus CNTV2DemoHevcCommon::CreateAiffFile(const char* pFileName, uint32_t numChannels, uint32_t maxFrames, uint32_t bufferSize)
{
    //  Close file if already open
    if (mAiffFd != NULL)
    {
        CloseAiffFile();
    }
    
    //  Open binary output
    mAiffFd = fopen(pFileName, "wb");
    if (mAiffFd == NULL)
    {
        return AJA_STATUS_OPEN;
    }
    
    //  Allocate copy buffer
    mAiffWriteBuffer = new uint8_t [bufferSize];
    if (mAiffWriteBuffer == NULL)
    {
        return AJA_STATUS_OPEN;
    }

    //  Initialize channels and counts for header
    mAiffTotalSize = 0;
    mAiffNumSamples = 0;
    mAiffNumChannels = numChannels;
    mMaxAiffFrames = maxFrames;
    
    //  Write AIFF header with 0 sizes
    WriteAiffHeader ();
    
    return AJA_STATUS_SUCCESS;
}


void CNTV2DemoHevcCommon::CloseAiffFile(void)
{
    if (mAiffFd != NULL)
    {
        //  Rewrite the header with final sizes
        WriteAiffHeader ();
        fclose(mAiffFd);
        mAiffFd = NULL;
        mMaxAiffFrames = 0;
    }
    
    if (mAiffWriteBuffer != NULL)
    {
        delete [] mAiffWriteBuffer;
        mAiffWriteBuffer = NULL;
    }
}


void CNTV2DemoHevcCommon::WriteAiffHeader(void)
{
    if (mAiffFd == NULL) return;
    
    fseek (mAiffFd, 0, SEEK_SET);
    
    //  Write the form chunk
    fprintf (mAiffFd,"FORM");
    fputc ((int)((mAiffTotalSize >> 24) & 0xff), mAiffFd);      //  Total file size
    fputc ((int)((mAiffTotalSize >> 16) & 0xff), mAiffFd);
    fputc ((int)((mAiffTotalSize >> 8) & 0xff), mAiffFd);
    fputc ((int)((mAiffTotalSize >> 0) & 0xff), mAiffFd);
    fprintf (mAiffFd,"AIFF");
    
    mAiffTotalSize = 4;
    
    //  Write the common chunk
    fprintf (mAiffFd,"COMM");
    fputc (0, mAiffFd);                                         //  Chunk size
    fputc (0, mAiffFd);
    fputc (0, mAiffFd);
    fputc (18, mAiffFd);
    fputc(0, mAiffFd);                                          //  Number of channels
    fputc (mAiffNumChannels, mAiffFd);
    fputc ((int)((mAiffNumSamples >> 24) & 0xff), mAiffFd);     //  Number of samples
    fputc ((int)((mAiffNumSamples >> 16) & 0xff), mAiffFd);
    fputc ((int)((mAiffNumSamples >> 8) & 0xff), mAiffFd);
    fputc ((int)((mAiffNumSamples >> 0) & 0xff), mAiffFd);
    fputc (0, mAiffFd);                                         // 16 bit
    fputc (16, mAiffFd);
    fputc (0x40, mAiffFd);                                      // 48000 kHz (10 byte sample rate) */
    fputc (0x0e, mAiffFd);
    fputc (0xbb, mAiffFd);
    fputc (0x80, mAiffFd);
    fputc (0, mAiffFd);
    fputc (0, mAiffFd);
    fputc (0, mAiffFd);
    fputc (0, mAiffFd);
    fputc (0, mAiffFd);
    fputc (0, mAiffFd);
    
    mAiffTotalSize += 26;
    
    //  Compute SSND chunk size
    uint32_t chunkSize = 2*mAiffNumChannels*mAiffNumSamples + 8;
    
    //  Write the sound data chunk
    fprintf (mAiffFd,"SSND");
    fputc ((int)((chunkSize >> 24) & 0xff), mAiffFd);           //  Chunk size
    fputc ((int)((chunkSize >> 16) & 0xff), mAiffFd);
    fputc ((int)((chunkSize >> 8) & 0xff), mAiffFd);
    fputc ((int)((chunkSize >> 0) & 0xff), mAiffFd);
    fputc (0, mAiffFd);                                         //  Audio offset
    fputc (0, mAiffFd);
    fputc (0, mAiffFd);
    fputc (0, mAiffFd);
    fputc (0, mAiffFd);                                         //  Audio block size
    fputc (0, mAiffFd);
    fputc (0, mAiffFd);
    fputc (0, mAiffFd);
    
    mAiffTotalSize += 16;
    
    mAiffNumSamples = 0;
}


void CNTV2DemoHevcCommon::WriteAiffData(void* pBuffer, uint32_t numChannels, uint32_t numSamples)
{
    uint8_t* pAudio = (uint8_t*)pBuffer;
    uint8_t* pAiff = mAiffWriteBuffer;
    
    if ((mAiffFd == NULL) ||
        (pAiff == NULL) ||
        (numChannels == 0) ||
        (numSamples == 0) ||
        (mMaxAiffFrames == 0)) return;
    
    if((mMaxAiffFrames != 0xffffffff) && (mAiffFileFrameCount > mMaxAiffFrames))
    {
        fseek (mAiffFd, 0, SEEK_SET);
        mAiffFileFrameCount = 0;
    }
    
    //  Copy audio from AJA buffer to AIFF buffer
    for (uint32_t is = 0; is < numSamples; is++)
    {
        for (uint32_t ic = 0; ic < numChannels; ic++)
        {
            if (ic < mAiffNumChannels)
            {
                *pAiff++ = *(pAudio+3);
                *pAiff++ = *(pAudio+2);
            }
            pAudio += 4;
        }
    }
    
    //  Compute AIFF audio size
    uint32_t audioSize = 2*mAiffNumChannels*numSamples;
    
    //  Write audio to AIFF file
    fwrite(mAiffWriteBuffer, 1, audioSize, mAiffFd);
    
    //  Increment AIFF header counts
    mAiffNumSamples += numSamples;
    mAiffTotalSize += audioSize;
    
    mAiffFileFrameCount++;
}


AJAStatus CNTV2DemoHevcCommon::OpenYuv420File(const char* pFileName, const uint32_t width, const uint32_t height)
{
	struct stat fileStat;

    //  Close file if already open
    if (mYuvFd != NULL)
    {
        CloseYuv420File();
    }
    
    //  Open binary output
    mYuvFd = fopen(pFileName, "rb");
    if (mYuvFd == NULL)
    {
        return AJA_STATUS_OPEN;
    }

    // Get the size of the file using fstat
	fstat(fileno(mYuvFd), &fileStat);
	mYuvFileSize = fileStat.st_size;

    // Now save width, height and calculate frame size and number of frames
    mYuvFrameWidth = width;
    mYuvFrameHeight = height;
    mYuvFrameSize = (width * height) + ((width * height) / 2);
    mYuvNumTotalFrames = mYuvFileSize / mYuvFrameSize;
    
    return AJA_STATUS_SUCCESS;
}


void CNTV2DemoHevcCommon::CloseYuv420File(void)
{
    if (mYuvFd != NULL)
    {
        fclose(mYuvFd);
        mYuvFd = NULL;
        mYuvFileSize = 0;
        mYuvFrameWidth = 0;
        mYuvFrameHeight = 0;
        mYuvFrameSize = 0;
        mYuvNumTotalFrames = 0;
    }
}


AJAStatus CNTV2DemoHevcCommon::ReadYuv420Frame(void* pBuffer, uint32_t numFrame)
{
    size_t result;
    
    if (numFrame > mYuvNumTotalFrames)
    {
        return AJA_STATUS_RANGE;
    }
    
    // Seek to the frame
    //fseek(mYuvFd, (mYuvFrameSize * numFrame), SEEK_SET);

    // Read the Y plane
    result = fread(pBuffer, 1, mYuvFrameSize, mYuvFd);
    if (result != (mYuvFrameSize))
        return AJA_STATUS_FAIL;
    
    return AJA_STATUS_SUCCESS;
}


AJAStatus CNTV2DemoHevcCommon::ConvertYuv420FrameToNV12(void* pSrcBuffer, void* pDstBuffer, uint32_t bufferSize)
{
    // Sanity check make sure they pass in a proper sized buffer
    if (bufferSize != mYuvFrameSize)
        return AJA_STATUS_FAIL;
    
    // Copy over Y plane
    memcpy(pDstBuffer, pSrcBuffer, (mYuvFrameWidth * mYuvFrameHeight));
    
    char * srcCbPtr;
    char * srcCrPtr;
    char * destCbCrPtr;
    
    srcCbPtr = (char*)pSrcBuffer + (mYuvFrameWidth * mYuvFrameHeight);
    srcCrPtr = (char*)pSrcBuffer + (mYuvFrameWidth * mYuvFrameHeight) + ((mYuvFrameWidth * mYuvFrameHeight) / 4);
    destCbCrPtr = (char*)pDstBuffer + (mYuvFrameWidth * mYuvFrameHeight);
    
    // Now interleave Cb and Cr planes
    for (uint32_t i = 0; i< (mYuvFrameWidth * mYuvFrameHeight) / 4; i++)
    {
        *destCbCrPtr++ = *srcCbPtr++;
        *destCbCrPtr++ = *srcCrPtr++;
    }
    
    return AJA_STATUS_SUCCESS;
}


AJAStatus CNTV2DemoHevcCommon::CreateRawFile(const char* pFileName, uint32_t maxFrames)
{
    //  Close file if already open
    if (mRawFd != NULL)
    {
        CloseRawFile();
    }
    
    //  Open binary output
    mRawFd = fopen(pFileName, "wb");
    if (mRawFd == NULL)
    {
        return AJA_STATUS_OPEN;
    }
    mMaxRawFrames = maxFrames;
    
    return AJA_STATUS_SUCCESS;
}


void CNTV2DemoHevcCommon::CloseRawFile(void)
{
    if (mRawFd != NULL)
    {
        fclose(mRawFd);
        mRawFd = NULL;
        mMaxRawFrames = 0;
    }
}


void CNTV2DemoHevcCommon::WriteRawData(void* pBuffer, uint32_t bufferSize)
{
    if ((mRawFd == NULL) ||
        (pBuffer == NULL) ||
        (bufferSize == 0) ||
        (mMaxRawFrames == 0)) return;
    
    if((mMaxRawFrames != 0xffffffff) && (mRawFileFrameCount > mMaxRawFrames))
    {
        fseek (mRawFd, 0, SEEK_SET);
        mRawFileFrameCount = 0;
    }
    
    fwrite(pBuffer, 1, bufferSize, mRawFd);
    mRawFileFrameCount++;
}


uint32_t CNTV2DemoHevcCommon::AlignDataBuffer(void* pBuffer, uint32_t bufferSize, uint32_t dataSize, uint32_t alignBytes, uint8_t fill)
{
    if ((pBuffer == NULL) ||
        (bufferSize == 0) ||
        (dataSize == 0)) return 0;
    
    if	(alignBytes == 0) return dataSize;
    
    // round up to aligned size
    uint32_t alignSize = ((dataSize - 1)/alignBytes + 1)*alignBytes;
    if (alignSize > bufferSize) return dataSize;
    
    // fill to aligned size
    uint8_t* pData = (uint8_t*)pBuffer;
    for (uint32_t i = dataSize; i < alignSize; i++)
    {
        pData[i] = fill;
    }
    
    return alignSize;
}


AJAStatus CNTV2DemoHevcCommon::DetermineInputFormat(NTV2VideoFormat sdiFormat, bool quad, NTV2VideoFormat& videoFormat)
{
    if (sdiFormat == NTV2_FORMAT_UNKNOWN)
        return AJA_STATUS_FAIL;
    
    switch (sdiFormat)
    {
        case NTV2_FORMAT_1080p_5000_A:
        case NTV2_FORMAT_1080p_5000_B:
            videoFormat = NTV2_FORMAT_1080p_5000_A;
            if (quad) videoFormat = NTV2_FORMAT_4x1920x1080p_5000;
            break;
        case NTV2_FORMAT_1080p_5994_A:
        case NTV2_FORMAT_1080p_5994_B:
            videoFormat = NTV2_FORMAT_1080p_5994_A;
            if (quad) videoFormat = NTV2_FORMAT_4x1920x1080p_5994;
            break;
        case NTV2_FORMAT_1080p_6000_A:
        case NTV2_FORMAT_1080p_6000_B:
            videoFormat = NTV2_FORMAT_1080p_6000_A;
            if (quad) videoFormat = NTV2_FORMAT_4x1920x1080p_6000;
            break;
        default:
            videoFormat = sdiFormat;
            break;
    }
    
    return AJA_STATUS_SUCCESS;
}


AJAStatus CNTV2DemoHevcCommon::SetupHEVC (CNTV2m31 * pM31, M31VideoPreset preset, M31Channel encodeChannel, bool multiStream, bool withInfo)
{
    HevcMainState   mainState;
    HevcEncodeMode  encodeMode;
    HevcVinState    vInState;
    HevcEhState     ehState;
    
    if (multiStream)
    {
        pM31->GetMainState(&mainState, &encodeMode);
        if ((mainState != Hevc_MainState_Encode) || (encodeMode != Hevc_EncodeMode_Multiple))
        {
            // Here we need to start up the M31 so we reset the part then go into the init state
            if (!pM31->Reset())
            { cerr << "## ERROR:  Reset of M31 failed" << endl;  return AJA_STATUS_INITIALIZE; }
            
            // After a reset we should be in the boot state so lets check this
            pM31->GetMainState(&mainState);
            if (mainState != Hevc_MainState_Boot)
            { cerr << "## ERROR:  Not in boot state after reset" << endl;  return AJA_STATUS_INITIALIZE; }
            
            // Now go to the init state
            if (!pM31->ChangeMainState(Hevc_MainState_Init, Hevc_EncodeMode_Multiple))
            { cerr << "## ERROR:  ChangeMainState to init failed" << endl;  return AJA_STATUS_INITIALIZE; }
            
            pM31->GetMainState(&mainState);
            if (mainState != Hevc_MainState_Init)
            { cerr << "## ERROR:  Not in init state after change" << endl;  return AJA_STATUS_INITIALIZE; }
            
            // Now lets configure the device for a given preset.  First we must clear out all of the params which
            // is necessary since the param space is basically uninitialized memory.
            pM31->ClearAllParams();
            
            // Load and set common params for all channels
            if (!pM31->SetupCommonParams(preset, M31_CH0))
            { cerr << "## ERROR:  SetCommonParams failed ch0 " << endl;  return AJA_STATUS_INITIALIZE; }
            
            // Change state to encode
            if (!pM31->ChangeMainState(Hevc_MainState_Encode, Hevc_EncodeMode_Multiple))
            { cerr << "## ERROR:  ChangeMainState to encode failed" << endl;  return AJA_STATUS_INITIALIZE; }
            
            pM31->GetMainState(&mainState);
            if (mainState != Hevc_MainState_Encode)
            { cerr << "## ERROR:  Not in encode state after change" << endl;  return AJA_STATUS_INITIALIZE; }
        }
        
        // Write out stream params
        if (!pM31->SetupVIParams(preset, encodeChannel))
        { cerr << "## ERROR:  SetupVIParams failed" << endl;  return AJA_STATUS_INITIALIZE; }
        if (!pM31->SetupVInParams(preset, encodeChannel))
        { cerr << "## ERROR:  SetupVinParams failed" << endl;  return AJA_STATUS_INITIALIZE; }
        if (!pM31->SetupVAParams(preset, encodeChannel))
        { cerr << "## ERROR:  SetupVAParams failed" << endl;  return AJA_STATUS_INITIALIZE; }
        if (!pM31->SetupEHParams(preset, encodeChannel))
        { cerr << "## ERROR:  SetupEHParams failed" << endl;  return AJA_STATUS_INITIALIZE; }
        
        if (withInfo)
        {
            // Enable picture information
            if (!pM31->mpM31VInParam->SetPTSMode(M31_PTSModeHost, (M31VirtualChannel)encodeChannel))
            { cerr << "## ERROR:  SetPTSMode failed" << endl;  return AJA_STATUS_INITIALIZE; }
        }
        
        // Now that we have setup the M31 lets change the VIn and EH states for channel 0 to start
        if (!pM31->ChangeVInState(Hevc_VinState_Start, encodeChannel))
        { cerr << "## ERROR:  ChangeVInState failed" << endl;  return AJA_STATUS_INITIALIZE; }
        
        pM31->GetVInState(&vInState, encodeChannel);
        if (vInState != Hevc_VinState_Start)
        {cout << "## VIn didn't start = '" << vInState << endl; }
        
        if (!pM31->ChangeEHState(Hevc_EhState_Start, encodeChannel))
        { cerr << "## ERROR:  ChangeEHState failed" << endl;  return AJA_STATUS_INITIALIZE; }
        
        pM31->GetEHState(&ehState, encodeChannel);
        if (ehState != Hevc_EhState_Start)
        { cout << "## EH didn't start = '" << ehState << endl; }
    }
    else
    {
        // if we are in the init state assume that last stop was good
        // otherwise reset the codec
        pM31->GetMainState(&mainState, &encodeMode);
        if ((mainState != Hevc_MainState_Init) || (encodeMode != Hevc_EncodeMode_Single))
        {
            // Here we need to start up the M31 so we reset the part then go into the init state
            if (!pM31->Reset())
            { cerr << "## ERROR:  Reset of M31 failed" << endl;  return AJA_STATUS_INITIALIZE; }
            
            // After a reset we should be in the boot state so lets check this
            pM31->GetMainState(&mainState);
            if (mainState != Hevc_MainState_Boot)
            { cerr << "## ERROR:  Not in boot state after reset" << endl;  return AJA_STATUS_INITIALIZE; }
            
            // Now go to the init state
            if (!pM31->ChangeMainState(Hevc_MainState_Init, Hevc_EncodeMode_Single))
            { cerr << "## ERROR:  ChangeMainState to init failed" << endl;  return AJA_STATUS_INITIALIZE; }
            
            pM31->GetMainState(&mainState);
            if (mainState != Hevc_MainState_Init)
            { cerr << "## ERROR:  Not in init state after change" << endl;  return AJA_STATUS_INITIALIZE; }
        }
        
        // Now lets configure the device for a given preset.  First we must clear out all of the params which
        // is necessary since the param space is basically uninitialized memory.
        pM31->ClearAllParams();
        
        // Now load params for M31 preset into local structures in CNTV2m31
        if (!pM31->LoadAllParams(preset))
        { cerr << "## ERROR:  LoadAllPresets failed" << endl;  return AJA_STATUS_INITIALIZE; }
        
        // Here is where you can alter params sent to the M31 because all of these structures are public
        
        // Write out all of the params to each of the 4 physical channels
        if (!pM31->SetAllParams(M31_CH0))
        { cerr << "## ERROR:  SetVideoPreset failed ch0 " << endl;  return AJA_STATUS_INITIALIZE; }
        
        if (!pM31->SetAllParams(M31_CH1))
        { cerr << "## ERROR:  SetVideoPreset failed ch1 " << endl;  return AJA_STATUS_INITIALIZE; }
        
        if (!pM31->SetAllParams(M31_CH2))
        { cerr << "## ERROR:  SetVideoPreset failed ch1 " << endl;  return AJA_STATUS_INITIALIZE; }
        
        if (!pM31->SetAllParams(M31_CH3))
        { cerr << "## ERROR:  SetVideoPreset failed ch1 " << endl;  return AJA_STATUS_INITIALIZE; }
        
        if (withInfo)
        {
            // Enable picture information
            if (!pM31->mpM31VInParam->SetPTSMode(M31_PTSModeHost, (M31VirtualChannel)M31_CH0))
            { cerr << "## ERROR:  SetPTSMode failed" << endl;  return AJA_STATUS_INITIALIZE; }
        }
        
        // Change state to encode
        if (!pM31->ChangeMainState(Hevc_MainState_Encode, Hevc_EncodeMode_Single))
        { cerr << "## ERROR:  ChangeMainState to encode failed" << endl;  return AJA_STATUS_INITIALIZE; }
        
        pM31->GetMainState(&mainState);
        if (mainState != Hevc_MainState_Encode)
        { cerr << "## ERROR:  Not in encode state after change" << endl;  return AJA_STATUS_INITIALIZE; }
        
        // Now that we have setup the M31 lets change the VIn and EH states for channel 0 to start
        if (!pM31->ChangeVInState(Hevc_VinState_Start, 0x01))
        { cerr << "## ERROR:  ChangeVInState failed" << endl;  return AJA_STATUS_INITIALIZE; }
        
        pM31->GetVInState(&vInState, M31_CH0);
        if (vInState != Hevc_VinState_Start)
        {cout << "## VIn didn't start = '" << vInState << endl; }
        
        if (!pM31->ChangeEHState(Hevc_EhState_Start, 0x01))
        { cerr << "## ERROR:  ChangeEHState failed" << endl;  return AJA_STATUS_INITIALIZE; }
        
        pM31->GetEHState(&ehState, M31_CH0);
        if (ehState != Hevc_EhState_Start)
        {cout << "## EH didn't start = '" << ehState << endl; }
    }
    
    return AJA_STATUS_SUCCESS;
}


