////////////////////////////////////////////////////////////
//
//	Copyright (C) 2003, 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.
//
////////////////////////////////////////////////////////////

#ifndef NTV2CAPTURE_H
#define NTV2CAPTURE_H
 
#include <string>

#include "ntv2card.h"
#define HANDLE HANDLE2
#include "qapplication.h"
#include "qclipboard.h"
#include "qimage.h"

typedef struct {
	ULWord numPixels,numLines,linePitch;
	ULWord* YCbCrBuffer;
} YCbCrCaptureBuffer;

class CNTV2Capture : public CNTV2Card 
{
public: // Construct - Destruct
	CNTV2Capture(UWord boardNumber, bool displayErrorMessage, UWord ulBoardType);
	~CNTV2Capture();

public: // Methods
	virtual bool SetBoard(UWord boardNumber);

	bool SetCaptureSource(NTV2CaptureSource source);
	void SetCaptureDestination(NTV2CaptureDestination destination);
	void SetCaptureDestinationWithFileName();
	void SetCaptureFileName(std::string& fileName);
	void SetCaptureFileName(const char* fileName);
	void SetCaptureSize(UWord percent);
	void SetCaptureQuality(UWord percent);
	void SetCaptureMode(NTV2CaptureMode captureMode);
	void SetupRouting();
	bool StartPreview();
	bool StopPreview();
	bool DoCapture();

	void CopyFrameToQImage(NTV2Channel channel,ULWord frameNumber,bool alpha);
	bool CopyFrameToYCbCrFile(NTV2Channel channel,
					          ULWord frameNumber,
						      bool alpha=false,
						      ULWord alphaFrameNumber=0,
						      bool makeThumbnail=false);
	bool WriteQImageToDestination();
	QImage* GetQImage();


protected:  // Methods


protected:  // Data
	NTV2CaptureSource      _captureSource;
	NTV2CaptureDestination _captureDestination;
	UWord                  _captureSize;
	UWord                  _captureQuality;
	bool                   _preview;
	NTV2CaptureMode        _captureMode;            
	bool                   _convertAspect;
	ULWord                 _captureTime;                 // milliseconds
	std::string            _fileName;

	// Save <-> Restore variables
	NTV2ReferenceSource    _referenceSource;
	NTV2FrameBufferFormat  _ch1FrameBufferFormat;  
	
	ULWord                 _vidProcCtrl;
	ULWord                 _vidProcXptCtrl;
	ULWord                 _mixCoefficient;

	// Capture buffer if not YCbCr
	QImage                _qImage;

private:

};

/////////////////////////////////////////////////////////////////////////////
#endif
