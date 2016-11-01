/**
	@file		ntv2videofile.h
	@brief		Declares the CNTV2VideoFile class.
	@copyright	(C) 2011-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef __NTV2__VIDEOFILE__
#define __NTV2__VIDEOFILE__

#include "AJATypes.h"
#include "ntv2enums.h"

#include <string>
#include <set>

/**
	@brief	This is a base class for video file reading/writing.
			To implement a specific video file format, subclass this
			and override the pure-virtual functions.
**/
class CNTV2VideoFile
{
public:
	CNTV2VideoFile();
	virtual ~CNTV2VideoFile();
	
	virtual bool Open(const std::string& fileName, bool readMode) = 0;
	
	virtual bool StartWriting() = 0;
	virtual void WriteBuffer(UByte* buffer, ULWord size) = 0;
	virtual void WriteAudioBuffer(UByte* buffer, ULWord size) = 0;
	virtual void WriteTcBuffer(UByte* buffer, ULWord size) = 0;
	virtual void FinishWriting() = 0;
	
	virtual bool StartReading() = 0;
	virtual void GoToFrame(UWord frameNumber) = 0;
	virtual void ReadBuffer(UByte* buffer, ULWord size) = 0;
	virtual void ReadAudioBuffer(UByte* buffer, ULWord size) = 0;
	virtual void ReadTcBuffer(UByte* buffer, ULWord size) = 0;
	virtual void FinishReading() = 0;
	
	virtual void Close() = 0;
	
	virtual std::set<NTV2FrameBufferFormat> GetSupportedFrameBufferFormats() const;
	
private:
	bool _hasVideo;
    bool _hasAudio;
    bool _hasRP188;
    NTV2VideoFormat _videoFormat;
    NTV2FrameBufferFormat _frameBufferFormat;
	NTV2FrameRate _frameRate;
	ULWord64 _numFrames;
	
	NTV2AudioRate _audioRate;
	UByte _audioNumChannels;
	ULWord64 _audioNumSamples;

protected:
	// A utility function for computing likely framerate constant
	// from the numerical framerate as a double.
	NTV2FrameRate GetFrameRate(double fps) const;
	
	// A utility function for guessing likely video format given
	// framerate, whether the video is 1080 and whether it is
	// progressive.
	NTV2VideoFormat GetVideoFormatFromFrameRate( NTV2FrameRate frameRate, bool tenEighty, bool progressive ) const;
	
public:
	////////////
    // Setters
	
    // video
    void SetHasVideo(bool b);
    void SetVideoFormat(NTV2VideoFormat vf);
    void SetFrameBufferFormat(NTV2FrameBufferFormat fbf);
	void SetFrameRate(NTV2FrameRate fr);
    void SetNumFrames(ULWord64 nf);
	
    // audio
    void SetHasAudio(bool b);
    void SetAudioRate(NTV2AudioRate ar);
	void SetAudioNumChannels(UByte nc);
	void SetAudioNumSamples(ULWord64 ans);
	
    // RP188
    void SetHasRP188(bool b);
	
	
	////////////
    // Getters
	
	// video
    bool HasVideo() const;
    bool HasAudio() const;
    bool HasRP188() const;
    NTV2VideoFormat GetVideoFormat() const;
    NTV2FrameBufferFormat GetFrameBufferFormat() const;
	NTV2FrameRate GetFrameRate() const;
	ULWord64 GetNumFrames() const;
	
    ULWord GetVideoFrameSize() const;
	ULWord GetVideoWriteSize() const;
    ULWord GetVideoDiskSize() const;
    ULWord GetVideoRate() const;
	
    // audio
    NTV2AudioRate GetAudioRate() const;
	UByte GetAudioNumChannels() const;
    ULWord64 GetAudioNumSamples() const;
	ULWord GetAudioSampleRate() const;
	ULWord GetAudioSampleSize() const;
};	//	CNTV2VideoFile

#endif  //   __NTV2__VIDEOFILE__
