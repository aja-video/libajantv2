/*
  Module: NTV2PlaybackfromDisk.h

  For a description of how to use this class please refer to NTV2PlaybackfromDisk.cpp

*/


#ifndef _NTV2PLAYBACKFROMDISK_H
#define _NTV2PLAYBACKFROMDISK_H

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2active.h"
#include "ntv2card.h"
#include "ntv2circularbuffer.h"
#include "ntv2xenacircularbuffer.h"
#include "ntv2diskio.h"
#include "ntv2rp188.h"          //  has definition of CRP188 class
#include "performancecounter.h"
#include "ntv2utils.h"
#include "xena2routing.h"

#include <string>


class CNTV2PlaybackFromDisk : public ActiveObject
{
public:
	CNTV2PlaybackFromDisk(UWord boardNumber,
					NTV2Channel channel,
					char *fileName,
					bool bPlaybackAudio,
                    bool bWithRP188 = false,
					bool bDualLinkOutput = false,
					NTV2DeviceType boardType= BOARDTYPE_UNKNOWN
			)
		: _boardNumber(boardNumber),
		  _channel(channel),
		  _bPlaybackAudio(bPlaybackAudio),
          _bUseRP188(bWithRP188),
		  _bDualLinkOutput(bDualLinkOutput),
		  _boardType(boardType),
		  _pause(false),
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
		_nextAudioOutputAddress = 0;
		_pause = false;
		_thread.Resume ();
        
	}

								            ~CNTV2PlaybackFromDisk() {  Kill();}
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
  
	void									TogglePause();
	void						            Pause() {_pause = !_pause;}
	bool						            GetPause() { return _pause; }
	void						            NextFrame();
	void						            PreviousFrame();
	void						            FirstFrame();
	void						            LastFrame();


    static DWORD WINAPI						PlaybackThread ( void *pArg);
	
protected:                                  
    virtual void				            InitThread ();
    virtual void				            Loop ();
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
	void						            DMAAudio();
	void						            ResetAudioOutput();
//	void						            DetectAudioFailure();
	bool						            TimeToTransferAVtoBoard();
                                            
	CNTV2Card					            _ntv2Card;
	UWord						            _boardNumber;
	NTV2DeviceType							_boardType;
	NTV2Channel					            _channel;
    std::string                             _fileName;	
    NTV2FrameRate                           _frameRate;  
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
    ULWord                                  _numAudioChannelsInFile;
	ULWord									_numAudioChannelsOnBoard;
	bool						            _bPlaybackAudio;
	bool									_bWithRP188;
    bool                                    _bUseRP188;
	ULWord						            _playDelay;
    CXenaFile<XENA_FILE_INFO>	            _XenaXtc;
    CXenaFile<XENA_FILE_INFO>	            _XenaXAud;
    CXenaFile<XENA_FILE_INFO>	            _XenaXVid;
    NTV2FrameDimensions                     _frameBufferSize;
    AUTOCIRCULATE_TRANSFER_STRUCT           _transferStruct;
    AUTOCIRCULATE_TRANSFER_STATUS_STRUCT    _transferStatusStruct;
	bool									_bDualLinkOutput;	   ///desired state
	bool									_dualLinkOutputEnable; ///saved state

	AVDataBuffer                            _avDataBuffer[NTV2_CIRCBUFFER_SIZE];
	CXenaCircularBuffer<AVDataBuffer*>		_playbackCircularBuffer;

	bool									_pause;
	
	bool						            _active;

	CXena2Routing							_xena2Router;
private:


	
};

#endif
