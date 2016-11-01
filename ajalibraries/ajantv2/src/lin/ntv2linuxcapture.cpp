// ntv2linuxcapture.cpp : implementation file
// 
// Copyright (C) 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.
//

#include "ntv2boardfeatures.h"
#include "ntv2linuxcapture.h"
#include "ntv2vidprocmasks.h"
#include "ntv2utils.h"
#include "ntv2transcode.h"

#include <stdlib.h>
#include <fstream>
using namespace std;

/////////////////////////////////////////////////////////////////////////////
// CNTV2Capture

CNTV2Capture::CNTV2Capture(UWord boardNumber, bool displayErrorMessage, UWord ulBoardType)
:  CNTV2Card(boardNumber, displayErrorMessage, ulBoardType)
{

	_preview = false;

	_captureSource = NTV2CAPTURESOURCE_INPUT1;

	_captureDestination = NTV2CAPTUREDESTINATION_YUVFILE;
	_captureSize = 100;
	_captureQuality = 50;
	_captureMode = NTV2CAPTUREMODE_FRAME;  
	_convertAspect = false;
	_captureTime = 100; 
	_fileName = "/tmp/foo.yuv";
	

}

CNTV2Capture::~CNTV2Capture()
{
	if ( _preview == true )
	{
		StopPreview();
	}

	// board closed by CNTV2Card destructor

}



bool CNTV2Capture::SetBoard(UWord boardNumber)
{


	if ( _preview == true )
	{
		StopPreview();
	}

	_boardNumber = boardNumber;

	Open(boardNumber);

	if ( _preview == true )
	{
		StartPreview();
	}

	return BoardOpened();

}

bool CNTV2Capture::SetCaptureSource(NTV2CaptureSource source)
{
	bool status = true;

	if ( _captureSource != source )
	{
		if ( _preview == true )
		{
			StopPreview();
			_captureSource = source;
			StartPreview();
		}
		else
		{
			_captureSource = source;
		}
	}

	return status;
}

void CNTV2Capture::SetCaptureDestination(NTV2CaptureDestination destination)
{
	_captureDestination = destination;
}

void CNTV2Capture::SetCaptureDestinationWithFileName()
{
	// Set Destination based on filename
	if ( _fileName.find(".yuv") == (_fileName.length()-4) )
	{
		_captureDestination = NTV2CAPTUREDESTINATION_YUVFILE;
	}
	else if ( _fileName.find(".bmp") == (_fileName.length()-4) )
	{
		_captureDestination = NTV2CAPTUREDESTINATION_BMPFILE;
	}
	else if ( _fileName.find(".jpg") == (_fileName.length()-4) )
	{
		_captureDestination = NTV2CAPTUREDESTINATION_JPEGFILE;
	}
	else if ( _fileName.find(".png") == (_fileName.length()-4) )
	{
		_captureDestination = NTV2CAPTUREDESTINATION_PNGFILE;
	}
	else
	{
		_captureDestination = NTV2CAPTUREDESTINATION_CLIPBOARD;
	}


}


void CNTV2Capture::SetCaptureFileName(std::string& fileName)
{
	_fileName = fileName;
	SetCaptureDestinationWithFileName();
}

void CNTV2Capture::SetCaptureFileName(const char* fileName)
{
	_fileName = fileName;
	SetCaptureDestinationWithFileName();
}

void CNTV2Capture::SetCaptureSize(UWord percent)
{
	if ( percent > 5 && percent < 200 )
	{
		_captureSize = percent;
	}
	else
	{
		_captureSize = 100;
	}
}

void CNTV2Capture::SetCaptureQuality(UWord percent)
{
	if ( percent > 5 && percent < 100 )
	{
		_captureQuality = percent;
	}
	else
	{
		_captureQuality = 50;
	}
}

void CNTV2Capture::SetCaptureMode(NTV2CaptureMode captureMode)
{
	_captureMode = captureMode;
}


bool CNTV2Capture::StartPreview()
{
	if ( !BoardOpened() )
		return false;


	if ( _preview != true )
	{
		if( NTV2BoardNeedsRoutingSetup(GetBoardID()) )
		{
			SetupRouting();
		}

		// Save Routing Information 
		ReadVideoProcessingControl(&_vidProcCtrl);
		ReadVideoProcessingControlCrosspoint(&_vidProcXptCtrl);
		ReadMixerCoefficient(&_mixCoefficient);
		ULWord vidProcCtrl = _vidProcCtrl;
		ULWord vidProcXptCtrl = _vidProcXptCtrl;
		GetReferenceSource(&_referenceSource);

		switch ( _captureSource )
		{
		  case NTV2CAPTURESOURCE_INPUT1:
			SetReferenceSource(NTV2_REFERENCE_INPUT1);
			// route input1 to output 1
			vidProcCtrl &= ~(VIDPROCMUX1MASK + VIDPROCMUX2MASK + VIDPROCMUX3MASK);
			vidProcCtrl |= (BIT_0 + BIT_2); // mix with full foreground video
			vidProcXptCtrl &= ~(FGVCROSSPOINTMASK);
			vidProcXptCtrl |= (BIT_1); // Input 1
			break;
		  case NTV2CAPTURESOURCE_INPUT2:
			SetReferenceSource(NTV2_REFERENCE_INPUT2);
			// route input2 to output 2
			vidProcCtrl &= ~(VIDPROCMUX4MASK + VIDPROCMUX5MASK);
			vidProcXptCtrl &= ~(BGVCROSSPOINTMASK);
			vidProcXptCtrl |= (BIT_4 + BIT_5); // Input 1
			break;

		  case NTV2CAPTURESOURCE_INPUT1_PLUS_INPUT2:
			SetReferenceSource(NTV2_REFERENCE_INPUT1);
			// route input1 to output 1
			vidProcCtrl &= ~(VIDPROCMUX1MASK + VIDPROCMUX2MASK + VIDPROCMUX3MASK);
			vidProcCtrl |= (BIT_0 + BIT_2); // mix with full foreground video
			vidProcXptCtrl &= ~(FGVCROSSPOINTMASK);
			vidProcXptCtrl |= (BIT_1); // Input 1
			// route input2 to output 2
			vidProcCtrl &= ~(VIDPROCMUX4MASK + VIDPROCMUX5MASK);
			vidProcXptCtrl &= ~(BGVCROSSPOINTMASK);
			vidProcXptCtrl |= (BIT_4 + BIT_5); // Input 1
			break;
		
		  case NTV2CAPTURESOURCE_FRAMEBUFFER: // Unsupported
	  		return false;
		}


		WriteMixerCoefficient(FIXED_ONE);
		WriteVideoProcessingControl(vidProcCtrl);
		WriteVideoProcessingControlCrosspoint(vidProcXptCtrl);

		_preview = true;
	}
	
	return true;
}


bool CNTV2Capture::StopPreview()
{

	if ( !BoardOpened() )
		return false;

	if ( _preview == true )
	{
		if( NTV2BoardNeedsRoutingSetup(GetBoardID()) )
		{
			ClearRouting();
		}

		// Restore Routing Information
		WriteVideoProcessingControl(_vidProcCtrl);
		WriteVideoProcessingControlCrosspoint(_vidProcXptCtrl);
		WriteMixerCoefficient(_mixCoefficient);
		SetReferenceSource(_referenceSource);

		switch ( _captureSource )
		{
		  case NTV2CAPTURESOURCE_INPUT1:
			SetMode(NTV2_CHANNEL1 ,NTV2_MODE_DISPLAY);
		    break;

		  case NTV2CAPTURESOURCE_INPUT2:
			SetMode(NTV2_CHANNEL2 ,NTV2_MODE_DISPLAY);
		    break;

		  case NTV2CAPTURESOURCE_INPUT1_PLUS_INPUT2:
			SetMode(NTV2_CHANNEL1 ,NTV2_MODE_DISPLAY);
			SetMode(NTV2_CHANNEL2 ,NTV2_MODE_DISPLAY);
		    break;

		  case NTV2CAPTURESOURCE_FRAMEBUFFER: // Unsupported
		  	return false;
		}

		SleepMs(30);
		_preview = false;
	}
	return true;
}

bool CNTV2Capture::DoCapture()
{
	if ( !BoardOpened() )
		return false;

	bool status = true;
	ULWord ch1OutFrame,ch2OutFrame,ch1PCIAccessFrame,ch2PCIAccessFrame;
	NTV2Channel captureChannel;
	ULWord captureFrame,alphaFrame;
	bool alpha;
	ULWord vidProcCtrl,vidProcXptCtrl,mixCoefficient;
	NTV2ReferenceSource referenceSource;

	// Save ReferenceSource
	GetReferenceSource(&referenceSource);

	// Save Routing Information 
	ReadVideoProcessingControl(&vidProcCtrl);
	ReadVideoProcessingControlCrosspoint(&vidProcXptCtrl);
	ReadMixerCoefficient(&mixCoefficient);
	GetOutputFrame(NTV2_CHANNEL1,&ch1OutFrame);
	GetOutputFrame(NTV2_CHANNEL2,&ch2OutFrame);
	GetPCIAccessFrame(NTV2_CHANNEL1,&ch1PCIAccessFrame);
	GetPCIAccessFrame(NTV2_CHANNEL2,&ch2PCIAccessFrame);
	SetAlphaFromInput2Bit(0);
	
	switch ( _captureSource )
	{
	case NTV2CAPTURESOURCE_INPUT1:
		SetReferenceSource(NTV2_REFERENCE_INPUT1);
		SetAlphaFromInput2Bit(0);
		SetInputFrame(NTV2_CHANNEL1,ch1OutFrame);

		// always capture YCbCr
		SetFrameBufferFormat(NTV2_CHANNEL1, NTV2_FBF_10BIT_YCBCR);
		SetMode(NTV2_CHANNEL1 ,NTV2_MODE_CAPTURE);
		SleepMs(_captureTime);
		SetMode(NTV2_CHANNEL1 ,NTV2_MODE_DISPLAY);
		SleepMs(60);
		if ( _preview )
		{
			// Send Channel 1 out of output 1
			ULWord tempvidProcXptCtrl = vidProcXptCtrl;
		    tempvidProcXptCtrl &= ~(FGVCROSSPOINTMASK);
			WriteVideoProcessingControlCrosspoint(tempvidProcXptCtrl);

		}

		alpha = false;
		captureChannel = NTV2_CHANNEL1;
		captureFrame = ch1OutFrame;
	    break;

	case NTV2CAPTURESOURCE_INPUT2:
		SetReferenceSource(NTV2_REFERENCE_INPUT2);
		SetAlphaFromInput2Bit(0);
		SetPCIAccessFrame(NTV2_CHANNEL2,ch2OutFrame);
		SetInputFrame(NTV2_CHANNEL2,ch2OutFrame);
		SetMode(NTV2_CHANNEL2 ,NTV2_MODE_CAPTURE);
		SleepMs(_captureTime);
		SetMode(NTV2_CHANNEL2 ,NTV2_MODE_DISPLAY);
		SleepMs(60);
		if ( _preview )
		{
			// Send Channel 2 out of output 2
			ULWord tempvidProcXptCtrl = vidProcXptCtrl;
		    tempvidProcXptCtrl &= ~(BGVCROSSPOINTMASK);
		    tempvidProcXptCtrl |= (BIT_4 ); 
			WriteVideoProcessingControlCrosspoint(tempvidProcXptCtrl);

		}
		alpha = false;
		captureChannel = NTV2_CHANNEL2;
		captureFrame = ch2OutFrame;
	    break;

	case NTV2CAPTURESOURCE_INPUT1_PLUS_INPUT2:
		SetReferenceSource(NTV2_REFERENCE_INPUT1);
		SetPCIAccessFrame(NTV2_CHANNEL1,ch1OutFrame);
		SetInputFrame(NTV2_CHANNEL1,ch1OutFrame);
		SetPCIAccessFrame(NTV2_CHANNEL2,ch2OutFrame);
		SetInputFrame(NTV2_CHANNEL2,ch2OutFrame);
		if ( _captureDestination  == NTV2CAPTUREDESTINATION_YUVFILE ||
			 _captureDestination  == NTV2CAPTUREDESTINATION_FRAMEBUFFERONLY )
		{
			SetFrameBufferFormat(NTV2_CHANNEL1, NTV2_FBF_10BIT_YCBCR);
		}
		else
		{
			SetAlphaFromInput2Bit(1); // Tell hardware to get alpha from Input 2
			SetFrameBufferFormat(NTV2_CHANNEL1, NTV2_FBF_ARGB);
		}
		SetMode(NTV2_CHANNEL1 ,NTV2_MODE_CAPTURE);
		SetMode(NTV2_CHANNEL2 ,NTV2_MODE_CAPTURE);
		SleepMs(_captureTime);
		SetMode(NTV2_CHANNEL1 ,NTV2_MODE_DISPLAY);
		SetMode(NTV2_CHANNEL2 ,NTV2_MODE_DISPLAY);
		SleepMs(60);
		if ( _preview )
		{
			// Send Channel 1 out of output 1
			// Send Channel 2 out of output 2
			ULWord tempvidProcXptCtrl = vidProcXptCtrl;
		    tempvidProcXptCtrl &= ~(FGVCROSSPOINTMASK);
		    // route input2 to output 2
		    tempvidProcXptCtrl &= ~(BGVCROSSPOINTMASK);
		    tempvidProcXptCtrl |= (BIT_4); 
			WriteVideoProcessingControlCrosspoint(tempvidProcXptCtrl);
		}
		alpha = true;
		captureChannel = NTV2_CHANNEL1;
		captureFrame = ch1OutFrame;	    
		alphaFrame = ch2OutFrame;	    
		break;


	default:
		return false;

	}
	SetReferenceSource(referenceSource);


	// Capture's Done...Sitting in framebuffer on card...Now Send to Destination
	if ( _captureDestination  == NTV2CAPTUREDESTINATION_YUVFILE )
	{
		CopyFrameToYCbCrFile(captureChannel,captureFrame,alpha,alphaFrame);
	}
	else
	{	
		CopyFrameToQImage(captureChannel,captureFrame,alpha);
		WriteQImageToDestination();
	}

	// Put Things back
	SetPCIAccessFrame(NTV2_CHANNEL1,ch1PCIAccessFrame);
	SetPCIAccessFrame(NTV2_CHANNEL2,ch2PCIAccessFrame);
	WriteVideoProcessingControl(vidProcCtrl);
	WriteVideoProcessingControlCrosspoint(vidProcXptCtrl);
	WriteMixerCoefficient(mixCoefficient);

	return status;
}



bool CNTV2Capture::CopyFrameToYCbCrFile(NTV2Channel channel,
								        ULWord frameNumber,
								        bool alpha,
								        ULWord alphaFrameNumber,
								        bool makeThumbnail)
{
	bool status = true;
	string currentExtension;
	ULWord* frameBuffer;
	ULWord linePitch;   // in 32 bit words
	ULWord numPixels;
	ULWord numLines;
	NTV2Standard standard;

	GetStandard(&standard);
	switch ( standard )
	{
	case NTV2_STANDARD_1080:
		numLines = HD_NUMACTIVELINES_1080;
		numPixels = HD_NUMCOMPONENTPIXELS_1080;
		linePitch = HD_YCBCRLINEPITCH_1080;
		break;
	case NTV2_STANDARD_720:
		numLines = HD_NUMACTIVELINES_720;
		numPixels = HD_NUMCOMPONENTPIXELS_720;
		linePitch = HD_YCBCRLINEPITCH_720;
		break;
	case NTV2_STANDARD_525:
		numLines = NUMACTIVELINES_525;
		numPixels = NUMCOMPONENTPIXELS;
		linePitch = YCBCRLINEPITCH_SD;
		break;
	case NTV2_STANDARD_625:
		numLines = NUMACTIVELINES_625;
		numPixels = NUMCOMPONENTPIXELS;
		linePitch = YCBCRLINEPITCH_SD;
		break;
	case NTV2_STANDARD_1080p:	// Not supported
	case NTV2_STANDARD_2K: 		// Not supported
	default: 
		return false;
	}


	ULWord oldPCIAccessFrame;
	GetPCIAccessFrame(channel,&oldPCIAccessFrame);
	SetPCIAccessFrame(channel,frameNumber);
	if( NTV2BoardCanDoPIO(GetBoardID()) )
	{
		MapFrameBuffers();
		GetBaseAddress(channel, &frameBuffer);
	}
	else
	{
		frameBuffer = (ULWord*) malloc( numLines * linePitch * 4);
		DmaReadFrame(NTV2_DMA_FIRST_AVAILABLE, frameNumber, frameBuffer, numLines * linePitch * 4 );
	}

	ULWord* dataBuffer = new ULWord[linePitch];

	ofstream outFile(_fileName.c_str(),ios::out | ios::binary);

	if ( outFile )
	{
		if ( _captureMode == NTV2CAPTUREMODE_FIELD )
		{
			for ( UWord lineCount = 0; lineCount < numLines; lineCount += 2 )
			{	
				memcpy(dataBuffer,frameBuffer,linePitch*4);
				outFile.write((char*)dataBuffer,linePitch*4);
				outFile.write((char*)dataBuffer,linePitch*4);
				frameBuffer += (2*linePitch);
			}
		}
		else
		{
			outFile.write((char*)frameBuffer,linePitch*4*numLines);
		}

		SetPCIAccessFrame(channel,oldPCIAccessFrame);
		outFile.close();

		if ( alpha )
		{
			GetPCIAccessFrame(NTV2_CHANNEL2,&oldPCIAccessFrame);
			SetPCIAccessFrame(NTV2_CHANNEL2,alphaFrameNumber);
			GetBaseAddress(NTV2_CHANNEL2, &frameBuffer);
			ULWord* dataBuffer = new ULWord[linePitch];
			string keyFileName = _fileName;
			int pos = keyFileName.rfind('.');
			keyFileName.insert(pos,".Key");

			ofstream outFile(keyFileName.c_str(),ios::out | ios::binary);
			
			if ( outFile )
			{
				if ( _captureMode == NTV2CAPTUREMODE_FIELD )
				{
					for ( UWord lineCount = 0; lineCount < numLines; lineCount += 2 )
					{	
						memcpy(dataBuffer,frameBuffer,linePitch*4);
						outFile.write((char*)dataBuffer,linePitch*4);
						outFile.write((char*)dataBuffer,linePitch*4);
						frameBuffer += (2*linePitch);
					}
				}
				else
				{
					outFile.write((char*)frameBuffer,linePitch*4*numLines);
				}

			}
			outFile.close();
			SetPCIAccessFrame(NTV2_CHANNEL2,oldPCIAccessFrame);
		}

		delete [] dataBuffer;

		SetPCIAccessFrame(channel,oldPCIAccessFrame);

	}

	if( NTV2BoardCanDoPIO(GetBoardID()) )
	{
		UnmapFrameBuffers();
	}
	else
	{
		delete [] frameBuffer;
	}

	return status;
}

void CNTV2Capture::CopyFrameToQImage(NTV2Channel channel,ULWord frameNumber,bool alpha)
{
	ULWord* frameBuffer;
	ULWord* frameBufferBase;
	ULWord linePitch;   // in 32 bit words
	NTV2Standard standard;
	ULWord oldPCIAccessFrame;

	GetStandard(&standard);
	switch ( standard )
	{
	case NTV2_STANDARD_1080:
		_qImage.create (HD_NUMCOMPONENTPIXELS_1080, HD_NUMACTIVELINES_1080, 32);
		linePitch = HD_YCBCRLINEPITCH_1080;
		break;
	case NTV2_STANDARD_720:
		_qImage.create (HD_NUMCOMPONENTPIXELS_720, HD_NUMACTIVELINES_720, 32);
		linePitch = HD_YCBCRLINEPITCH_720;
		break;
	case NTV2_STANDARD_525:
		_qImage.create (NUMCOMPONENTPIXELS, NUMACTIVELINES_525, 32);
		linePitch = YCBCRLINEPITCH_SD;
		break;
	case NTV2_STANDARD_625:
		_qImage.create (NUMCOMPONENTPIXELS, NUMACTIVELINES_625, 32);
		linePitch = YCBCRLINEPITCH_SD;
		break;
	
	case NTV2_STANDARD_1080p:	// Not supported
	case NTV2_STANDARD_2K: 		// Not supported
	default: 
		return;
	}


	GetPCIAccessFrame(channel,&oldPCIAccessFrame);
	SetPCIAccessFrame(channel,frameNumber);
	if( NTV2BoardCanDoPIO(GetBoardID()) )
	{
		MapFrameBuffers();
		GetBaseAddress(channel, &frameBufferBase);
	}
	else
	{
		frameBufferBase = (ULWord*) malloc( _qImage.height() * linePitch * 4);
		DmaReadFrame(NTV2_DMA_FIRST_AVAILABLE, frameNumber, frameBufferBase, _qImage.height() * linePitch * 4 );
	}
	frameBuffer = frameBufferBase;

	if ( alpha == true )
	{

		if ( _captureMode == NTV2CAPTUREMODE_FIELD )
		{
			ULWord* dataBuffer = new ULWord[_qImage.width()];

			for ( UWord lineCount = 0; lineCount < _qImage.height(); lineCount += 2 )
			{	
				memcpy(dataBuffer,frameBuffer,_qImage.width()*4);
				memcpy(_qImage.scanLine(lineCount),dataBuffer,_qImage.width()*4);
				memcpy(_qImage.scanLine(lineCount+1),dataBuffer,_qImage.width()*4);

				frameBuffer += (2*_qImage.width());

			}
			delete [] dataBuffer;
		}
		else
		{		
			for ( LWord lineCount = 0; lineCount < _qImage.height(); lineCount++ )
			{
				memcpy(_qImage.scanLine(lineCount),frameBuffer,_qImage.width()*4);
				frameBuffer += _qImage.width();
			}
		}

	}
	else
	{
		// No alpha, so it has been captured in YCbCr space
		// and needs colorspace conversion.
		ULWord* dataBuffer;
		UWord* YCbCrBuffer;
		dataBuffer = new ULWord[linePitch];
		YCbCrBuffer = new UWord[HD_NUMCOMPONENTPIXELS_1080*2];

		bool  bIsSD;
		IsSDStandard(&bIsSD);

		if ( _captureMode == NTV2CAPTUREMODE_FIELD )
		{

			for ( UWord lineCount = 0; lineCount < _qImage.height(); lineCount += 2 )
			{	
				memcpy(dataBuffer,frameBuffer,linePitch*4);
				UnPackLineData(dataBuffer,YCbCrBuffer,_qImage.width());
				ConvertLinetoRGB(YCbCrBuffer, (RGBAlphaPixel *)_qImage.scanLine(lineCount),_qImage.width(), bIsSD);
				memcpy(_qImage.scanLine(lineCount+1),_qImage.scanLine(lineCount),_qImage.width()*4);

				frameBuffer += (2*linePitch);

			}
		}
		else
		{		
			for ( LWord lineCount = 0; lineCount < _qImage.height(); lineCount++ )
			{
				memcpy(dataBuffer,frameBuffer,linePitch*4);
				UnPackLineData(dataBuffer,YCbCrBuffer,_qImage.width());
				ConvertLinetoRGB(YCbCrBuffer,(RGBAlphaPixel *)_qImage.scanLine(lineCount),_qImage.width(), bIsSD);

				frameBuffer += linePitch;
			}
		}

		delete [] dataBuffer;
		delete [] YCbCrBuffer;	
	}

	SetPCIAccessFrame(channel,oldPCIAccessFrame);

	if( NTV2BoardCanDoPIO(GetBoardID()) )
	{
		UnmapFrameBuffers();
	}
	else
	{
		delete [] frameBufferBase;
	}
}

bool CNTV2Capture::WriteQImageToDestination()
{


	if ( _captureSize != 100 )
	{
		int newHeight = (_qImage.height()*_captureSize/100);
		int newWidth = (_qImage.width()*_captureSize/100);
		_qImage = _qImage.smoothScale(newWidth,newHeight);
	}

	if ( _captureDestination == NTV2CAPTUREDESTINATION_CLIPBOARD )
	{
		QClipboard *cb = QApplication::clipboard();
		cb->setImage(_qImage);

	}
	else if ( _captureDestination == NTV2CAPTUREDESTINATION_BMPFILE )
	{
		_qImage.save(_fileName.c_str(),"BMP");
	}
	else if ( _captureDestination == NTV2CAPTUREDESTINATION_JPEGFILE )
	{
		_qImage.save(_fileName.c_str(),"JPEG");
	}
	else if ( _captureDestination == NTV2CAPTUREDESTINATION_PNGFILE )
	{
		_qImage.save(_fileName.c_str(),"PNG");
	}
 
	return true;
}



QImage* CNTV2Capture::GetQImage()
{
	return &_qImage;
}

void CNTV2Capture::SetupRouting()
{
	ULWord input;
	switch(_captureSource)
	{
	default:
	case NTV2CAPTURESOURCE_INPUT1:
		input = NTV2K2_XptSDIIn1;
		break;
	case NTV2CAPTURESOURCE_INPUT2:
		input = NTV2K2_XptSDIIn2;
		break;
	}

	//One way to route is to use the Xena2RoutingTable class
	CXena2Routing xena2Router;
	switch(_captureSource)
	{
	case NTV2CAPTURESOURCE_INPUT1:
		//If it is not 4K then we will route the user selected input to the frame buffer
		if(IsRGBFormat(_ch1FrameBufferFormat))
		{
			xena2Router.addWithValue(CSC1VidInputSelectEntry, input);
			xena2Router.addWithValue(FrameBuffer1InputSelectEntry, NTV2K2_XptCSC1VidRGB);
		}
		else
		{
			xena2Router.addWithValue(FrameBuffer1InputSelectEntry, input);
		}

		// Route E2E
		xena2Router.addWithValue(SDIOut1InputSelectEntry, input);

		// Check for bi-directional sdi
		if(NTV2BoardHasBiDirectionalSDI(GetBoardID()))
			SetSDITransmitEnable(NTV2_CHANNEL1, false);
		
		break;
	case NTV2CAPTURESOURCE_INPUT2:
		if(IsRGBFormat(_ch1FrameBufferFormat))
		{
			xena2Router.addWithValue(CSC2VidInputSelectEntry, input);
			xena2Router.addWithValue(FrameBuffer2InputSelectEntry, NTV2K2_XptCSC2VidRGB);
		}
		else
		{
			xena2Router.addWithValue(FrameBuffer2InputSelectEntry, input);
		}

		// Route E2E
		xena2Router.addWithValue(SDIOut2InputSelectEntry, input);

		// Check for bi-directional sdi
		if(NTV2BoardHasBiDirectionalSDI(GetBoardID()))
			SetSDITransmitEnable(NTV2_CHANNEL2, false);
		break;
	}

	OutputRoutingTable(xena2Router);
}

