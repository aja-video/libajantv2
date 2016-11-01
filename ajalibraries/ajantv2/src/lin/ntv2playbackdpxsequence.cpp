/*

  Module : NTV2PlaybackDPXSequence.cpp

  This software is provided by AJA Video, Inc. "AS IS" with no express or implied warranties.

  This software plays back dpx files.....in either NTV2_FBF_10BIT_DPX or NTV2_FBF_10BIT_DPX_LITTLEENDIAN
  modes only. Only supported by dual link capable boards.

  Note that it is important to call CNTV2PlaybackDPXSequence::Quit() before deleting the object.

*/


#include "ntv2playbackdpxsequence.h"
//#include "ntv2remotecontrol.h"
#include "ntv2utils.h"
#include "ntv2boardscan.h"
#include "ntv2boardfeatures.h"
#include "audiodefines.h"
#include "fixed.h"
#if defined(MSWindows)
#  include "cpu.h"
#endif
#include "ntv2debug.h"
#include <vector>

#if defined(AJALinux)
#  include "ntv2winlinhacks.h"
#  include <sys/types.h>
#  include <dirent.h>
#  include <stdio.h>
#  include <errno.h>
#endif


void CNTV2PlaybackDPXSequence::InitThread()
{
   	if ( _ntv2Card.Open(_boardNumber,false,_boardType) )
	{
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
void CNTV2PlaybackDPXSequence::Loop()
{
	if ( _ntv2Card.BoardOpened()  )
	{
		AVDataBuffer* playData;
		DWORD threadID;
#if defined(MSWindows)
		HANDLE hPlaybackThread = CreateThread(NULL,0,CNTV2PlaybackDPXSequence::PlaybackThread,this,0,&threadID);
#endif
#if defined (AJALinux)
		pthread_t hPlaybackThread;
		threadID = pthread_create(&hPlaybackThread, NULL, CNTV2PlaybackDPXSequence::PlaybackThread,this);
#endif
		_currentFrameNumber = 0;
		CRP188 rp188;
		RP188_STRUCT  rp188_struct;
		NTV2FrameRate frameRate;
		_ntv2Card.GetFrameRate(&frameRate);

		while ( !_isDying )
		{
			playData = _playbackCircularBuffer.StartProduceNextBuffer();
			if ( playData == NULL ) goto done;
			
			// Now Read from file(s)
			_fileName = _filenamList[_currentFrameNumber];
#if defined(MSWindows)
			HANDLE hDPX;
			hDPX = ::CreateFile(_fileName.c_str(),
								GENERIC_READ,
								0,NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING ,
								NULL);
			
			if( hDPX != INVALID_HANDLE_VALUE)
			{
				DWORD numBytesRead;
				if ( _twoKWindow)
				{
					::SetFilePointer(hDPX,_imageOffset+(_lineOffset*2048*4), NULL,FILE_BEGIN);
					::ReadFile(hDPX,_tempVideoBuffer,(2048*1080*4),&numBytesRead,NULL);
					ULWord* destBuffer = playData->videoBuffer;
					ULWord* sourceBuffer = _tempVideoBuffer;
					for ( int line = 0; line<1080; line++ )
					{
						::CopyMemory(destBuffer,&sourceBuffer[_pixelOffset],1920*4);
						destBuffer += 1920;
						sourceBuffer += 2048;
					}
				}
				else
				{
					::SetFilePointer(hDPX,_imageOffset, NULL,FILE_BEGIN);
					::ReadFile(hDPX,playData->videoBuffer,playData->videoBufferSize,&numBytesRead,NULL);

				}
				::CloseHandle(hDPX);
#else
			FILE *hDPX;
			hDPX = fopen(_fileName.c_str(), "r");
			if ( hDPX != NULL)
			{
				DWORD numBytesRead;
				if( _twoKWindow)
				{
					fseek(hDPX, _imageOffset+(_lineOffset*2048*4), SEEK_SET);
					numBytesRead = fread(_tempVideoBuffer, 1, (2048*1080*4), hDPX);
					ULWord* destBuffer = playData->videoBuffer;
					ULWord* sourceBuffer = _tempVideoBuffer;
					
					for ( int line = 0; line<1080; line++ )
					{
						memcpy(destBuffer, &sourceBuffer[_pixelOffset], 1920*4);
						destBuffer += 1920;
						sourceBuffer += 2048;
					}
				}
				else
				{
					fseek(hDPX, _imageOffset, SEEK_SET);
					numBytesRead = fread(playData->videoBuffer, 1, playData->videoBufferSize, hDPX);
				}
				fclose(hDPX);

#endif
				rp188.SetRP188(_currentFrameNumber,0,0,0,frameRate,true);
				rp188.GetRP188Reg(rp188_struct);
				playData->rp188Data = rp188_struct;
			}

			_playbackCircularBuffer.EndProduceNextBuffer(); // Signal Being done with buffer

			if ( !_pause )
				++_currentFrameNumber;
			if ( _currentFrameNumber >= _numFrames)
			{
				if( _loop )
					_currentFrameNumber = 0;
				else
					_isDying++;
			}
		

			
		}       
done:

#if defined(MSWindows)
		WaitForSingleObject(hPlaybackThread,INFINITE);
#else
		fprintf(stderr, "%s: Loop done, joining thread\n", __FUNCTION__);
		pthread_join(hPlaybackThread, NULL);
#endif
		
		for ( ULWord i=0; i< (_maxFrame-_minFrame-1); i++ )
		{
			delete [] _avDataBuffer[i].videoBuffer; 
			delete [] _avDataBuffer[i].audioBuffer;
		}

		delete [] _tempVideoBuffer;
  
	}
	
	_active = false;
}

void * WINAPI CNTV2PlaybackDPXSequence::PlaybackThread (void* pArg)
{
    CNTV2PlaybackDPXSequence * pPlayback = (CNTV2PlaybackDPXSequence *) pArg;
    pPlayback->PlaybackToBoard ();
    return 0;
}

// PlaybackToBoard()
// Empty circular buffer to board.
void CNTV2PlaybackDPXSequence::PlaybackToBoard()
{
	CPerformanceCounter pc;
	AVDataBuffer* playData;
	int _isDying = 0;
//	fprintf(stderr, "%s: _isDying: %d\n", __FUNCTION__, _isDying);
	while ( !_isDying )
	{
		AUTOCIRCULATE_STATUS_STRUCT acStatus;
		_ntv2Card.GetAutoCirculate(_channelSpec, &acStatus );
		_bufferLevel = acStatus.bufferLevel;
		_droppedFrames = acStatus.framesDropped;

		if ( _bufferLevel < (_numBuffersAvailable-1) )
		{
			playData = _playbackCircularBuffer.StartConsumeNextBuffer();
			if ( playData == NULL ) goto done;
pc.Start();			
			_transferStruct.videoBuffer = playData->videoBuffer;
			_transferStruct.videoBufferSize = playData->videoBufferSize;

			_transferStruct.rp188 = playData->rp188Data;

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

bool CNTV2PlaybackDPXSequence::SetupDiskFiles()
{

	_filenamList.clear();

#if defined(MSWindows)
	_splitpath(_fileName.c_str(),_currentDirectory,baseDirectory,baseFileName,extention);
	strcat(_currentDirectory,baseDirectory);
	::SetCurrentDirectory(_currentDirectory);
	_numFrames = 0;
	WIN32_FIND_DATA fileData;
	HANDLE hSearch = ::FindFirstFile("*.dpx",&fileData);
	while ( ::FindNextFile(hSearch,&fileData) != 0 )
	{
		_filenamList.push_back(fileData.cFileName);
		_numFrames++;
	}
	::FindClose(hSearch);
#endif

#if defined(AJALinux)
	char path[MAX_PATH];
	char full_name[MAX_PATH];
	char *path_part;
	char *prev_path_part;
	const char *sep = "/";
	int i;

	for(i=0; i < MAX_PATH;i++)
		path[i] = '\0';
	
	path_part = strtok((char *)_fileName.c_str(), sep);
	
	while (path_part != NULL)
	{
		prev_path_part = path_part;
		path_part = strtok(NULL, sep);
		if (path_part != NULL) {
			strcat(path, sep);
			strcat(path, prev_path_part);
		}
	}

	struct dirent **de;
	int n;
	n = scandir(path, &de, 0, alphasort);
	for(int i=0; i < n; i++) {
		if ( strstr(de[i]->d_name, ".dpx")!=NULL ) {
			strcpy(full_name, path);
			strcat(full_name, sep);
			strcat(full_name, de[i]->d_name);
			_filenamList.push_back(full_name);
			_numFrames++;
			fprintf(stderr, "%s: adding %s\n", __FUNCTION__, full_name);
			free(de[i]);
		}
	}
	// reset _fileName
	_fileName = _filenamList[0];
#endif
	return true;
}

bool CNTV2PlaybackDPXSequence::SetupChannelInfo()
{
    // Use user supplied value to set channelspec
	_channelSpec = NTV2CROSSPOINT_CHANNEL1;
	_minFrame = 0;
	_maxFrame = 6;
	_playDelay = 4;
	
	// if currently autocirculating on _channelSpec stop it.
	_ntv2Card.StopAutoCirculate( _channelSpec);

	_numBuffersAvailable = _maxFrame-_minFrame-1;
	return true;
}

void CNTV2PlaybackDPXSequence::SetupPlaybackVideo()
{

	// Open video file to get parameters 
	DpxHdr dpxFile(_fileName.c_str());
	_imageOffset = (ULWord)dpxFile.get_fi_image_offset();
	_numLines = (ULWord)dpxFile.get_ii_lines();
	_numPixels = (ULWord)dpxFile.get_ii_pixels();
	_imageSize = (ULWord)dpxFile.get_ii_image_size();
	float frameRate = dpxFile.get_film_frame_rate();
	if ( dpxFile.IsBigEndian())
	{
		_frameBufferFormat = NTV2_FBF_10BIT_DPX;
	}
	else
	{
		_frameBufferFormat = NTV2_FBF_10BIT_DPX_LITTLEENDIAN;
	}
	// Default Video Format
	NTV2VideoFormat videoFormat;
	_ntv2Card.GetVideoFormat(&videoFormat);
	
	if ( _numLines == 1556 && _numPixels == 2048 )
	{
		if ( _twoKWindow )
		{
			_imageSize = 1920*1080*4;
			if ( frameRate == 14.98)
				videoFormat = NTV2_FORMAT_1080psf_2398;
			else
				videoFormat = NTV2_FORMAT_1080psf_2400;

		}
		else
		{
			if ( frameRate == 14.98)
				videoFormat = NTV2_FORMAT_2K_1498;
			else
				videoFormat = NTV2_FORMAT_2K_1500;

		}
	}
	else 
	if ( _numLines == 1080 && _numPixels == 1920 )
	{
		_twoKWindow = false;
		_imageSize = 1920*1080*4;
		if ( frameRate == 23.98)
			videoFormat = NTV2_FORMAT_1080psf_2398;
		else
			videoFormat = NTV2_FORMAT_1080psf_2400;
	}
	else 
	if ( _numLines == 1080 && _numPixels == 2048 )
	{
		if ( _twoKWindow )
		{
			_imageSize = 1920*1080*4;
			if ( frameRate == 23.98)
				videoFormat = NTV2_FORMAT_1080psf_2398;
			else
				videoFormat = NTV2_FORMAT_1080psf_2400;

		}
		else
		{
			if ( frameRate == 23.98)
				videoFormat = NTV2_FORMAT_1080psf_2K_2398;
			else
				videoFormat = NTV2_FORMAT_1080psf_2K_2400;

		}

	}
	else
	if ( _numLines == 486 && _numPixels == 720)
	{
		_twoKWindow = false;
		videoFormat = NTV2_FORMAT_525_5994;
		_frameBufferFormat = NTV2_FBF_24BIT_RGB;
		_imageSize = 720*486*3;
	}
	else
	if ( _numLines == 576 && _numPixels == 720)
		videoFormat = NTV2_FORMAT_625_5000;

	else
	if ( _numLines == 720 && _numPixels == 1280)
	{
		_twoKWindow = false;
		videoFormat = NTV2_FORMAT_720p_5994;
	}


	_ntv2Card.SetVideoFormat(videoFormat);
	
	dpxFile.close();

	bool isProgressive;
	_ntv2Card.IsProgressiveStandard(&isProgressive);
	if ( isProgressive )
		_ntv2Card.SetRegisterWritemode(NTV2_REGWRITE_SYNCTOFIELD);// weird but true
	else
		_ntv2Card.SetRegisterWritemode(NTV2_REGWRITE_SYNCTOFRAME);

	
	::Sleep(100);

	_ntv2Card.SetMode(NTV2_CHANNEL1,NTV2_MODE_DISPLAY);

    _ntv2Card.SetFrameBufferFormat(NTV2_CHANNEL1, _frameBufferFormat);

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
	
	BuildRoutingTableForOutput(_xena2Router,NTV2_CHANNEL1,_frameBufferFormat,false,false,true);
	_ntv2Card.OutputRoutingTable(_xena2Router);

}

void CNTV2PlaybackDPXSequence::SetupAutoCirculate()
{
	// Start AutoCirculating and turn on audio if desired.
	memset ((void *) &_transferStruct, 0, sizeof(_transferStruct));
	_videoReadSize = _imageSize;
	_transferStruct.channelSpec = _channelSpec;
	_transferStruct.videoBuffer = NULL;
	_transferStruct.videoBufferSize   = _videoReadSize;
	_transferStruct.frameBufferFormat = _frameBufferFormat;
	_transferStruct.audioBuffer = NULL;
	_transferStruct.audioBufferSize = 0;
	
	//TODO: replace with whatever initialization the new member needs
	//_transferStruct.disableAudioDMA = FALSE; 
	_transferStruct.bDisableExtraAudioInfo = true;
	_transferStruct.audioStartSample = 0;
	_transferStruct.audioNumChannels = 0;
	_transferStruct.frameRepeatCount = 1;
	_transferStruct.desiredFrame = -1;

	_ntv2Card.InitAutoCirculate( _channelSpec, 
								 _minFrame, 
								 _maxFrame, 
								 false,
								 true,
								 false, 
								 false, 
								 0);    


}


void CNTV2PlaybackDPXSequence::SetupDiskCircularBuffer()
{
	_playbackCircularBuffer.Init(&_isDying);
	_tempVideoBuffer = NULL;

	for ( ULWord i=0; i<(_maxFrame-_minFrame-1); i++ )
	{
		_avDataBuffer[i].videoBuffer = new ULWord[_imageSize/4];
		_avDataBuffer[i].videoBufferSize = _imageSize;
		_avDataBuffer[i].audioBuffer = NULL;
		_avDataBuffer[i].audioBufferSize = 0;			// this will change each frame

		_playbackCircularBuffer.Add(&_avDataBuffer[i]);
	}

	if ( _twoKWindow )
	{
		_tempVideoBuffer = new ULWord[2048*1080*4];
	}

}

void CNTV2PlaybackDPXSequence::SetPixelOffset(LWord offset) 
{ 
	if ( offset < 0 )
		offset = 0;
	else
	if ( offset > 127 )
		_pixelOffset = 127;
	else
		_pixelOffset = offset; 

}
void CNTV2PlaybackDPXSequence::SetLineOffset(LWord offset) 
{ 
	if ( offset < 0 )
		offset = 0;
	else
	if ( offset > 475 )
		_lineOffset = 475;
	else
		_lineOffset = offset; 
	
}

void CNTV2PlaybackDPXSequence::TogglePause()
{
	_pause = !_pause;
}
void CNTV2PlaybackDPXSequence::NextFrame()
{
	if ( _pause )
	{
		if ( _currentFrameNumber >= _numFrames)
			_currentFrameNumber = _numFrames-1;
		else
			_currentFrameNumber++;
	}

}
void CNTV2PlaybackDPXSequence::PreviousFrame()
{
	if ( _pause )
	{
		if ( _currentFrameNumber > 0)
			--_currentFrameNumber;
	}

}

void CNTV2PlaybackDPXSequence::FirstFrame()
{
	if ( _pause )
	{
		if ( _currentFrameNumber >= _numFrames)
			_currentFrameNumber = _numFrames-1;
		else
			_currentFrameNumber = 0;;
	}

}

void CNTV2PlaybackDPXSequence::LastFrame()
{
	if ( _pause )
	{
		_currentFrameNumber = _numFrames-1;
	}
}

void CNTV2PlaybackDPXSequence::Quit()
{
	_isDying++;
	while ( _active )
		Sleep(100);
}

