/*
  Module: NTV2PlaybackfromDisk.h

  For a description of how to use this class please refer to NTV2PlaybackfromDisk.cpp

*/


#ifndef _NTV2PLAYBACKDPXSEQUENCE_H
#define _NTV2PLAYBACKDPXSEQUENCE_H

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2active.h"
#include "ntv2card.h"
#include "ntv2circularbuffer.h"
#include "ntv2xenacircularbuffer.h"
//#include "ntv2diskio.h"
#include "ntv2rp188.h"          //  has definition of CRP188 class
#include "performancecounter.h"
#include "ntv2utils.h"
#include "xena2routing.h"
#include "ntv2dpx.h"

#include <string>

#if defined(AJALinux)
#  include "ntv2winlinhacks.h"
#  include <stdio.h>
#endif

class CNTV2PlaybackDPXSequence : public ActiveObject
{
public:
	CNTV2PlaybackDPXSequence(UWord boardNumber,
					char *fileName,
					NTV2DeviceType boardType= BOARDTYPE_UNKNOWN,
					bool loop = true,
					bool window2K=true
			)
		: _boardNumber(boardNumber),
		  _boardType(boardType),
		  _loop(loop),
		  _twoKWindow(window2K),
		  _pixelOffset(64),
		  _lineOffset(238),
		  _tempVideoBuffer(NULL),
		  _active(true)

	{
        _fileName = fileName;
		_minSeconds = 0.0;
		_maxSeconds = 0.0;
		_curSeconds = 0.0;
		_numFramesPlayed = 0;
		_currentFrameNumber = 0;
		_numFrames = 0;
		_droppedFrames = 0;
		_bufferLevel = 0;
		_pause = false;
		_thread.Resume ();
        
		fprintf(stderr, "%s: _isDying: %d\n", __FUNCTION__, _isDying);

	}

	~CNTV2PlaybackDPXSequence() { Kill(); 
 }

	void						            Quit();
	bool						            Active() { return _active; }
                                            
	double						            GetMinSecondsPerLoop() { return _minSeconds; }
	double						            GetMaxSecondsPerLoop() { return _maxSeconds; }
	double									GetCurSecondsPerLoop() { return _curSeconds; }
	ULWord						            GetPlayDelayNumber()   { return _playDelay; }
	ULWord						            GetCurrentTransferFrame( void ) { return _currentTransferFrame ; }
	ULWord						            GetNumFramesPlayed( void ) { return _numFramesPlayed ; }
	ULWord						            GetCurrentFrameNumber( void ) { return _currentFrameNumber ; }
	ULWord						            GetNumFrames( void ) { return _numFrames ; }
	ULWord						            GetDroppedFrames()   { return _droppedFrames; }
	ULWord						            GetBufferLevel() { return _bufferLevel; }
    void									GetFileName(std::string& fileName) { fileName = _fileName ; }                                        
  
	void						            Pause() {_pause = !_pause;}
	bool									GetPause(){ return _pause; }
	void									TogglePause();
	void						            NextFrame();
	void						            PreviousFrame();
	void						            FirstFrame();
	void						            LastFrame();


	LWord									GetPixelOffset() { return _pixelOffset; }
	void									SetPixelOffset(LWord offset);
	LWord									GetLineOffset() { return _lineOffset; }
	void									SetLineOffset(LWord offset);

    static void * WINAPI						PlaybackThread ( void *pArg);
    virtual void				            Loop ();
	
protected:                                  
    virtual void				            InitThread ();
    virtual void				            FlushThread () {_active=false;}
 
	bool									SetupDiskFiles();
	bool									SetupChannelInfo();
	void									SetupPlaybackVideo();
	void									SetupAutoCirculate();
	void									SetupDiskCircularBuffer();
	
	void						            StartAudioPlayback();
	void						            StopAudioPlayback();
	bool						            CheckForMissedFrame();
	void						            PlaybackToBoard();
                                            
	CNTV2Card					            _ntv2Card;
	UWord						            _boardNumber;
	NTV2DeviceType							_boardType;
	NTV2Channel					            _channel;
    std::string                             _fileName;	
    NTV2FrameBufferFormat                   _frameBufferFormat;
    NTV2AudioRate                           _audioRate;
	ULWord*						            _pBaseVideoAddress;
	ULWord*                                 _pBaseAudioAddress;
    ULWord*                                 _pAudioDiskAddress;
	ULWord						            _videoReadSize;
	ULWord						            _audioReadSize;
	ULWord						            _nextAudioOutputAddress;
	ULWord                                  _minFrame;
	ULWord                                  _maxFrame;
	NTV2Crosspoint				            _channelSpec;	
	double                                  _minSeconds;
	double                                  _maxSeconds;
	double									_curSeconds;
	ULWord						            _currentTransferFrame;
	ULWord									_numBuffersAvailable;
	ULWord						            _numFramesPlayed;
	ULWord									_currentFrameNumber;
	ULWord						            _numFrames;
	ULWord						            _numFramesReadFromDisk;
	ULWord						            _bufferLevel;
	ULWord						            _droppedFrames;
	ULWord						            _playDelay;
    NTV2FrameDimensions                     _frameBufferSize;
    AUTOCIRCULATE_TRANSFER_STRUCT           _transferStruct;
    AUTOCIRCULATE_TRANSFER_STATUS_STRUCT    _transferStatusStruct;
	bool									_bDualLinkOutput;	   ///desired state
	bool									_dualLinkOutputEnable; ///saved state

	ULWord _imageOffset; 
	ULWord _numLines; 
	ULWord _numPixels; 
	ULWord _imageSize;
	char _currentDirectory[MAX_PATH];

	bool									_loop;
	bool									_twoKWindow;
	LWord									_pixelOffset;
	LWord									_lineOffset;
	ULWord*									_tempVideoBuffer;
	bool						            _active;

	AVDataBuffer                            _avDataBuffer[32];
	CXenaCircularBuffer<AVDataBuffer*>		_playbackCircularBuffer;
	XenaStringList							_filenamList;

	bool									_pause;

	CXena2Routing							_xena2Router;

private:


	
};

#endif
