/////////////////////////////////////////////////////////////////////////////
//
// CNTV2ScanConvert
// General Purpose Class to convert graphics to video to download to 
// NTV2 Card. This class derives from CNTVPCICard. 
// Uses Qt's QImage class to do most of the work
//
////////////////////////////////////////////////////////////
//
//	Copyright (C) 2003, 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.
//
////////////////////////////////////////////////////////////


#include "ntv2card.h"
#define HANDLE HANDLE2
#include "qimage.h"

/*********************************************************************/
/* Work Buffer related defines used by ScanConvert Class */
#define BUFFERLINEPITCH 5000
/* Work Buffer currently uses 5 lines */
#define NUMBUFFERLINES 11
#define WORKBUFFERSIZE (BUFFERLINEPITCH*NUMBUFFERLINES*sizeof(RGBAlphaPixel))

class CNTV2ScanConvert : public CNTV2Card 
{

public: // Construct/Destruct
	CNTV2ScanConvert(UWord boardNumber, bool displayErrorMessage, UWord ulBoardType);
	~CNTV2ScanConvert();


public:  // Methods

	bool ScanConvertQImage(QImage *pImage, 
  						   NTV2Channel channel = NTV2_CHANNEL1,
						   NTV2OutputFilter verticalFilter = NTV2OUTPUTFILTER_NONE,
						   bool convertAspect = false,
						   bool autoSize = false);


	bool ScanConvertFile(const char* fileName,
 						 NTV2Channel channel = NTV2_CHANNEL1,
						 NTV2OutputFilter verticalFilter = NTV2OUTPUTFILTER_NONE,
						 bool convertAspect = false,
						 bool autoSize = false);

 
	bool ScanConvertYUVFile(const char* fileName,
							NTV2Channel channel = NTV2_CHANNEL1,
 						    NTV2OutputFilter verticalFilter = NTV2OUTPUTFILTER_NONE);	

	bool ScanConvertYUVFileToQImage(char* fileName,
							        QImage* pImage);

protected:  // Methods

	bool SetupGeometry(RECT &inputRect,
		                  RECT &outputRect,
						  UWord &numLines,
						  UWord &numPixels,
						  UWord &frameBufferLinePitch,
						  bool &convertAspect,
						  bool autoSize);

	
	void Create10BitblackCbYCrLine(UWord* lineData);

protected: // Data
	char* _workBuffer;                 // allocated when first used

};

/////////////////////////////////////////////////////////////////////////////
