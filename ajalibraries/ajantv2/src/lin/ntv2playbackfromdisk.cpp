/*

  Module : NTV2PlaybackFromDisk.cpp

  This software is provided by AJA Video, Inc. "AS IS" with no express or implied warranties.

  Demo class to show the playback of data stored on disk for NTV2(KSD,KHD,Xena,Xena2)

  Please refer to the NTV2AVDemo project in the NTV2SDK for an example of how this class is
  used.


  At the core of this example is the autocirculate API. The CNTV2Card class supports 

//
//  autocirculate methods
//
	bool  InitAutoCirculate (NTV2Crosspoint channelSpec,

		LWord StartFrameNumber, 
		LWord EndFrameNumber,

		bool  bWithAudio = false,
		bool  bWithRP188 = false,
        bool  bFbfChange = false,
        bool  bFboChange = false,
		bool  bWithColorCorrection = false,
		bool  bWithVidProc = false,          
		bool  bWithCustomAncData=false)	;

	bool  StartAutoCirculate (NTV2Crosspoint channelSpec);
	bool  StopAutoCirculate (NTV2Crosspoint channelSpec);
	bool  AbortAutoCirculate (NTV2Crosspoint channelSpec);
	bool  PauseAutoCirculate (NTV2Crosspoint channelSpec, bool pPlayToPause);
	bool  GetAutoCirculate (NTV2Crosspoint channelSpec, AUTOCIRCULATE_STATUS_STRUCT* autoCirculateStatus);
	bool  GetFrameStamp (NTV2Crosspoint channelSpec, ULWord frameNum, FRAME_STAMP_STRUCT* pFrameStamp);
	bool  FlushAutoCirculate (NTV2Crosspoint channelSpec);
	bool  PrerollAutoCirculate (NTV2Crosspoint channelSpec, ULWord lPrerollFrames);
	bool  TransferWithAutoCirculate (PAUTOCIRCULATE_TRANSFER_STRUCT pTransferStruct,
                                          PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT pTransferStatusStruct);  


After the StartAutocirculate is called the Channel1 Output frame(in this case) will be changed every
frame by the driver so that it cycles through _minFrame to _maxFrame continuously until 
StopAutoCirclulate is called.
While autocirculating is happening in the driver the code below keeps the 
framebuffers(from _minFrame to _maxFrame) loaded up with as much video data 
for disk as possible. This minimizes the chances that another application 
that is a process hog will cause frames to be dropped. To help ensure this, its
best to run the application including this class at "Realtime" process priority. 


  Note that it is important to call CNTV2PlaybackFromDisk::Quit() before deleting the object.

*/


#include "ntv2playbackfromdisk.h"
//#include "ntv2remotecontrol.h"
#include "ntv2utils.h"
#include "ntv2boardscan.h"
#include "ntv2boardfeatures.h"
#include "audiodefines.h"
#include "fixed.h"
//#include "cpu.h"
#include "ntv2debug.h"
#include <vector>


void CNTV2PlaybackFromDisk::InitThread()
{
   	if ( _ntv2Card.Open(_boardNumber,false,_boardType) )
	{
		if ( !NTV2BoardCanDoAudio(_ntv2Card.GetBoardID()))
			_bPlaybackAudio = false;

		if ( !NTV2BoardCanDoDualLink(_ntv2Card.GetBoardID()))
			_bDualLinkOutput = false;

		if ( !SetupDiskFiles() )
		{
	        MessageBox( NULL, "Couldn't Open Disk File", NULL, NULL);
			_ntv2Card.Close();
			return;
		}
		if ( !SetupChannelInfo() )
		{
	        MessageBox( NULL, "Couldn't Get Channel", NULL, NULL);
			_ntv2Card.Close();
			return;
		}

		SetupPlaybackVideo();

		SetupAutoCirculate();

		SetupDiskCircularBuffer();

	}
	else
	{
		MessageBox( NULL, "Couldn't Open Board", NULL, NULL);
	}
}

// Loop
// This loop simply continues to fill up the circular buffer with video,audio 
// and rp188 data that is read from disk
void CNTV2PlaybackFromDisk::Loop()
{
	if ( _ntv2Card.BoardOpened() && (_XenaXVid.IsOpened()) )
	{
		AVDataBuffer* playData;
		DWORD threadID;
		HANDLE hPlaybackThread = CreateThread(NULL,0,CNTV2PlaybackFromDisk::PlaybackThread,this,0,&threadID);
		ULWord currentSample = 0;
		ULWord* tempAudioBuffer = new ULWord[NTV2_AUDIOSIZE_MAX/4];
		CRP188 rp188;
		RP188_STRUCT  rp188_struct;
		while ( !_isDying )
		{
			playData = _playbackCircularBuffer.StartProduceNextBuffer();
			if ( playData == NULL ) goto done;
			
			// Now Read from file(s)
			_XenaXVid.GoToFrame(_currentFrameNumber);
			_XenaXVid.ReadBuffer((UByte *) playData->videoBuffer, playData->videoBufferSize);
			if ( _bPlaybackAudio )
			{
				ULWord ulNumAudioSamps = GetAudioSamplesPerFrame(_frameRate, _audioRate, _numFramesReadFromDisk);
				_audioReadSize = ulNumAudioSamps * _numAudioChannelsInFile *NTV2_AUDIOSAMPLESIZE;
				if ( _numAudioChannelsInFile == _numAudioChannelsOnBoard)
				{
					// easy case
					_XenaXAud.ReadAudioBuffer((UByte *) playData->audioBuffer, _audioReadSize);
					playData->audioBufferSize = _audioReadSize;
				}
				else
				{
					// need to do some munging.
					_XenaXAud.ReadAudioBuffer((UByte *) tempAudioBuffer, _audioReadSize);
					ULWord* destPtr  = playData->audioBuffer;
					if ( _numAudioChannelsInFile < _numAudioChannelsOnBoard)
					{
						memset(destPtr,NTV2_AUDIOSIZE_MAX,0);
						ULWord* audioPtr = tempAudioBuffer;
						for ( ULWord count=0; count < ulNumAudioSamps; count++)
						{
							for ( ULWord channelNum = 0; channelNum < _numAudioChannelsInFile; channelNum++ )
							{
								destPtr[channelNum] = *audioPtr++;
							}
							destPtr += _numAudioChannelsOnBoard;
						}
						playData->audioBufferSize = ulNumAudioSamps*_numAudioChannelsOnBoard*NTV2_AUDIOSAMPLESIZE;
					}
					else
					{
						// _numAudioChannelsInFile > _numAudioChannelsOnBoard
						ULWord* audioPtr = tempAudioBuffer;
		 				for ( ULWord count=0; count < ulNumAudioSamps; count++)
						{
							for ( ULWord channelNum = 0; channelNum < _numAudioChannelsOnBoard; channelNum++ )
							{
								*destPtr++ = audioPtr[channelNum];
							}
							audioPtr += _numAudioChannelsInFile;
						}
						playData->audioBufferSize = ulNumAudioSamps*_numAudioChannelsOnBoard*NTV2_AUDIOSAMPLESIZE;
						
					}
				}
			}

			if (_bUseRP188 && _bWithRP188)
			{
				_XenaXtc.ReadTcBuffer((UByte *) &playData->rp188Data, sizeof (RP188_STRUCT));
			}
			else
			if (_bUseRP188 )
			{
				rp188.SetRP188(_numFramesReadFromDisk,0,0,0,_frameRate,true);
                rp188.GetRP188Reg(rp188_struct);
                playData->rp188Data = rp188_struct;
			}
			
			_playbackCircularBuffer.EndProduceNextBuffer(); // Signal Being done with buffer

			 // If we are at the end of our rope, rewind 
			if ( !_pause )
				++_currentFrameNumber;
			if ( ((++_numFramesReadFromDisk) % (_XenaXVid.GetNumFrames()-1)) == 0 )
			{
				if ( !_pause )
					_currentFrameNumber = 0;
				_XenaXVid.SetPosition((LWord64) DEFAULT_SECTOR_SIZE);

				if ( _bPlaybackAudio )
					_XenaXAud.SetPosition((LWord64) DEFAULT_SECTOR_SIZE);

				if (_bWithRP188)
					_XenaXtc.SetPosition((LWord64)0);
			}

			
		}       
done:

		WaitForSingleObject(hPlaybackThread,INFINITE);
		
		for ( int i=0; i<NTV2_CIRCBUFFER_SIZE; i++ )
		{
			delete [] _avDataBuffer[i].videoBuffer; 
			delete [] _avDataBuffer[i].audioBuffer;
		}

		delete [] tempAudioBuffer;

        _XenaXVid.CloseFile();
        _XenaXAud.CloseFile();
        _XenaXtc.CloseFile();

	}
	
	_active = false;
}

DWORD WINAPI CNTV2PlaybackFromDisk::PlaybackThread (void* pArg)
{
    CNTV2PlaybackFromDisk * pPlayback = (CNTV2PlaybackFromDisk *) pArg;
    pPlayback->PlaybackToBoard ();
    return 0;
}

// PlaybackToBoard()
// Empty circular buffer to board.
void CNTV2PlaybackFromDisk::PlaybackToBoard()
{
	CPerformanceCounter pc;
	bool lastpause = false;
	AVDataBuffer* playData;
	while ( !_isDying )
	{
		AUTOCIRCULATE_STATUS_STRUCT acStatus;
		_ntv2Card.GetAutoCirculate(_channelSpec, &acStatus );
		_bufferLevel = acStatus.bufferLevel;
		_droppedFrames = acStatus.framesDropped;



		if ( _bufferLevel < (_numBuffersAvailable-1))
		{
			playData = _playbackCircularBuffer.StartConsumeNextBuffer();
			if ( playData == NULL ) goto done;
			pc.Start();			
			_transferStruct.videoBuffer = playData->videoBuffer;
			_transferStruct.videoBufferSize = playData->videoBufferSize;
			if ( _bPlaybackAudio )
			{
				_transferStruct.audioBuffer = playData->audioBuffer;
				_transferStruct.audioBufferSize = playData->audioBufferSize;
			}

			if (_bUseRP188)
			{
				_transferStruct.rp188 = playData->rp188Data;
				CRP188 rp188;
				rp188.SetRP188 (_transferStruct.rp188);
				string rp188str;
				rp188.GetRP188Str (rp188str);
				//odprintf("%s\n",rp188str.c_str());
			}

 			_ntv2Card.TransferWithAutoCirculate(&_transferStruct, &_transferStatusStruct);

			_numFramesPlayed++;	
			if ( _numFramesPlayed == _playDelay)
			{
				_ntv2Card.StartAutoCirculate( _channelSpec );
			}
			_playbackCircularBuffer.EndConsumeNextBuffer(); // Signal Being done with buffer

pc.Stop();
_curSeconds = pc.GetSeconds();
_minSeconds = pc.GetMinSeconds();
_maxSeconds = pc.GetMaxSeconds();
		

		}
		else
		{
			_ntv2Card.WaitForVerticalInterrupt();
		}
	}
done:
	_ntv2Card.StopAutoCirculate( _channelSpec );
	_ntv2Card.SetDualLinkOutputEnable(_dualLinkOutputEnable);

}


bool CNTV2PlaybackFromDisk::SetupDiskFiles()
{
	// open video file 
	if (_XenaXVid.OpenFile (_fileName, true) == false)
		return false;
	_XenaXVid.AddRef();
	_XenaXVid.ReadHeader();

	// open audio file if desired
    _pBaseAudioAddress = NULL;
    _audioReadSize = 0;
	_numFramesReadFromDisk = 0;
	_numFrames = _XenaXVid.GetNumFrames();
    if ( _XenaXVid.HasAudio() && _bPlaybackAudio )
    {
        if (_XenaXAud.OpenAudioFile(_fileName, true))
        {
			_XenaXAud.AddRef();
			_XenaXAud.ReadHeader();
			
			_numAudioChannelsInFile = _XenaXAud.GetAudioChannels();
			
			_ntv2Card.GetFrameRate(&_frameRate);
			_ntv2Card.GetAudioRate(&_audioRate);

		}
		else
			_bPlaybackAudio = false;
	}   
	else
		_bPlaybackAudio = false;
	
	
	// Open RP-188 file if it has one and we want to play it back
	_bWithRP188 = _XenaXVid.HasRP188();
	if ( _bWithRP188 && _bUseRP188)
	{
		if (_XenaXtc.OpenTcFile(_fileName, true))
		{
			_XenaXtc.AddRef();
			_ntv2Card.SetRP188Mode(_channel,NTV2_RP188_OUTPUT);
		}
	}
	
	return true;
}

bool CNTV2PlaybackFromDisk::SetupChannelInfo()
{
    // Use user supplied value to set channelspec
	if ( _channel == NTV2_CHANNEL1 )
	{
		_channelSpec = NTV2CROSSPOINT_CHANNEL1;
		_minFrame = 0;
		_maxFrame = 7;
	}
	else
	{
		_channelSpec = NTV2CROSSPOINT_CHANNEL2;
		_minFrame = 8;
		_maxFrame = 14;
	}
	_playDelay = 1;
	
	AUTOCIRCULATE_STATUS_STRUCT acStatus;
	_ntv2Card.GetAutoCirculate(_channelSpec, &acStatus );         
	if ( acStatus.state != NTV2_AUTOCIRCULATE_DISABLED  )
		return false;
		
	// if currently autocirculating on _channelSpec stop it.
	_ntv2Card.StopAutoCirculate( _channelSpec);

	// Insure _maxFrame is legal
 	_numBuffersAvailable = _maxFrame - _minFrame - 1;	// number of buffers available to preload
														// -1 is so we don't overwrite currently outputting frame.
														// can be changed if audio is being played back.

	return true;
}

void CNTV2PlaybackFromDisk::SetupPlaybackVideo()
{
	// Try and keep in sync with Control Panel
	CNTV2RemoteControl remoteControl(NULL);
    NTV2VideoFormat videoFormat = _XenaXVid.GetVideoFormat();

	NTV2VideoFormat currentVideoFormat;
    _ntv2Card.GetVideoFormat(&currentVideoFormat);


	if ( currentVideoFormat != videoFormat)
	{
		if ( remoteControl.SetVideoFormat(_boardNumber, videoFormat) == false)
			_ntv2Card.SetVideoFormat(videoFormat);
	}
	_ntv2Card.GetFrameRate(&_frameRate);
	bool tenEightyP = false;
	switch ( videoFormat )
	{
	case NTV2_FORMAT_1080p_5000:
		_frameRate = NTV2_FRAMERATE_5000;
		tenEightyP = true;
		break;
	case NTV2_FORMAT_1080p_5994:
		_frameRate = NTV2_FRAMERATE_5994;
		tenEightyP = true;
		break;
	case NTV2_FORMAT_1080p_6000:
		_frameRate = NTV2_FRAMERATE_6000;
		tenEightyP = true;
		break;
	}



	bool isProgressive;
	_ntv2Card.IsProgressiveStandard(&isProgressive);
	if ( isProgressive )
		_ntv2Card.SetRegisterWritemode(NTV2_REGWRITE_SYNCTOFIELD);// weird but true
	else
		_ntv2Card.SetRegisterWritemode(NTV2_REGWRITE_SYNCTOFRAME);

	
	::Sleep(1000);

	_ntv2Card.SetMode(_channel,NTV2_MODE_DISPLAY);
    _frameBufferFormat = _XenaXVid.GetFrameBufferFormat();
    _ntv2Card.SetFrameBufferFormat(_channel, _frameBufferFormat);

	_ntv2Card.GetNumberAudioChannels(&_numAudioChannelsOnBoard);

	_ntv2Card.SubscribeOutputVerticalEvent();

	// save state
	_ntv2Card.GetDualLinkOutputEnable(&_dualLinkOutputEnable);
	if ( _bDualLinkOutput ) 
	{
		_ntv2Card.SetDualLinkOutputEnable(true);
	}
	else
	{
		_ntv2Card.SetDualLinkOutputEnable(false);
	}
	
	if( NTV2BoardNeedsRoutingSetup(_ntv2Card.GetBoardID() ))
	{
		BuildRoutingTableForOutput(_xena2Router,_channel,_frameBufferFormat,false,false,_bDualLinkOutput);
		if ( tenEightyP)
			BuildRoutingTableForOutput(_xena2Router,NTV2_CHANNEL2,_frameBufferFormat,false,false,_bDualLinkOutput);
		_ntv2Card.OutputRoutingTable(_xena2Router);
	}
}

void CNTV2PlaybackFromDisk::SetupAutoCirculate()
{
	// Start AutoCirculating and turn on audio if desired.
	memset ((void *) &_transferStruct, 0, sizeof(_transferStruct));
	_videoReadSize = _XenaXVid.GetVideoDiskSize();
	_transferStruct.channelSpec = _channelSpec;
	_transferStruct.videoBuffer = NULL;
	_transferStruct.videoBufferSize   = _videoReadSize;
	_transferStruct.frameBufferFormat = _frameBufferFormat;
	_transferStruct.audioBuffer = NULL;
	_transferStruct.audioBufferSize = 0;

	_transferStruct.disableAudioDMA = FALSE;
	_transferStruct.bDisableExtraAudioInfo = true;
	_transferStruct.audioStartSample = 0;
	_transferStruct.audioNumChannels = _numAudioChannelsOnBoard;
	_transferStruct.frameRepeatCount = 1;
	_transferStruct.desiredFrame = -1;

	_ntv2Card.InitAutoCirculate( _channelSpec, 
								 _minFrame, 
								 _maxFrame, 
								 _bPlaybackAudio,
								 _bUseRP188,
								 false, 
								 false, 
								 0);    


}


void CNTV2PlaybackFromDisk::SetupDiskCircularBuffer()
{
	_playbackCircularBuffer.Init(&_isDying);
	
	for ( int i=0; i<NTV2_CIRCBUFFER_SIZE; i++ )
	{
		_avDataBuffer[i].videoBuffer = new ULWord[_XenaXVid.GetVideoDiskSize()/4];
		_avDataBuffer[i].videoBufferSize = _XenaXVid.GetVideoDiskSize();
		_avDataBuffer[i].audioBuffer = new ULWord[NTV2_AUDIOSIZE_MAX/4];
		_avDataBuffer[i].audioBufferSize = NTV2_AUDIOSIZE_MAX;			// this will change each frame

		_playbackCircularBuffer.Add(&_avDataBuffer[i]);
	}

}



void CNTV2PlaybackFromDisk::TogglePause()
{
	_pause = !_pause;
}

void CNTV2PlaybackFromDisk::NextFrame()
{
	if ( _pause )
	{
		if ( _currentFrameNumber >= _numFrames)
			_currentFrameNumber = _numFrames-1;
		else
			_currentFrameNumber++;
	}

}
void CNTV2PlaybackFromDisk::PreviousFrame()
{
	if ( _pause )
	{
		if ( _currentFrameNumber > 0)
			--_currentFrameNumber;
	}

}

void CNTV2PlaybackFromDisk::FirstFrame()
{
	if ( _pause )
	{
		if ( _currentFrameNumber >= _numFrames)
			_currentFrameNumber = _numFrames-1;
		else
			_currentFrameNumber = 0;;
	}

}
void CNTV2PlaybackFromDisk::LastFrame()
{
	if ( _pause )
	{
		_currentFrameNumber = _numFrames-1;
	}

}



void CNTV2PlaybackFromDisk::Quit()
{
	_isDying++;
	while ( _active )
		Sleep(100);
}


