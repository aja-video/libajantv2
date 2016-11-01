//---------------------------------------------------------------------------------------------------------------------
//  NTV2OglDPXReader.cpp
//
//	Copyright (C) 2012 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
//---------------------------------------------------------------------------------------------------------------------
#include "ntv2ogldpxreader.h"
#include "ajastuff/system/systemtime.h"
#include "ajastuff/common/videoutilities.h"
#include "ajastuff/system/file_io.h"
#include "ajastuff/common/dpx_hdr.h"
#include "ntv2boardscan.h"
#include "ntv2utils.h"

#include <QDate>
#include <string>
#include <sstream>
#include <iomanip>

using std::string;





NTV2OglDPXReader::NTV2OglDPXReader(int deviceIndex, QGLWidget*		captureContext, IOglTransfer *gpuTransfer)
: CVideoProcessor<COglObject>(gpuTransfer), mCaptureContext(captureContext)
{
	mWidth    = 0;
	mHeight   = 0;
	mNumFiles = 0;
	mPixelFormat    = AJA_PixelFormat_RGB_DPX;
	mCurrentFrameNumber = 0;
	/// Hard code it for now.
	mDirectoryPath  = "V:/mib";

	mAbort = false;

	mNumOglObjects = 3;
}

NTV2OglDPXReader::~NTV2OglDPXReader()
{
	wait();
	Deinit();
}


bool NTV2OglDPXReader::Init()
{
	if(mGpuQueue[OUTQ] == NULL || mGpuTransfer == NULL)
		return false;
	
	// Open Directory
	// Make list of files
	// 

	setupFiles();
	getFileInfo(mFileList[0].absoluteFilePath());  // note to bilbo - this information not used

	//create GPU objects 
	mCaptureContext->makeCurrent();	
	GpuObjectDesc desc;
	desc._format = GL_RGBA;
	desc._type = GL_UNSIGNED_BYTE;
	desc._height = mHeight;
	desc._width = mWidth;
	desc._useTexture = true;
	desc._useRenderToTexture = false;	
	mOglObjects = new COglObject[mNumOglObjects];
	for(int i = 0; i< mNumOglObjects; i ++)
	{		
		//init each object. This must be done when the OpenGL context is current
		mOglObjects[i].Init(desc);	
		//regsiter the object with the Gpu transfer. This must be done when the OpenGL context is current
		mGpuTransfer->RegisterTexture(&mOglObjects[i]);	
		//finally, add it to the circular buffer 
		mGpuQueue[OUTQ]->Add(&mOglObjects[i]);
	}
	
	
	// create the system memory objects
	//we need to regsiter the objects with the Gpu transfer. This must be done when the OpenGL context is current
	CpuObjectDesc desc2;
	desc2._format = GL_RGBA;
	desc2._type = GL_UNSIGNED_BYTE;
	desc2._height = mHeight;
	desc2._width = mWidth;
	desc2._useTexture = true;
	desc2._stride = desc2._width*oglFormatToNumChannels(desc2._format)*oglTypeToBytes(desc2._type);
	mGpuTransfer->GetGpuPreferredAllocationConstants(&desc2._addressAlignment, &desc2._strideAlignment);
	 
	mCpuObjects = new CCpuObject[mNumOglObjects];
	for(int i = 0; i< mNumOglObjects; i ++)
	{		
		//init each object
		mCpuObjects[i].Init(desc2);	
		//regsiter the object with the Gpu transfer. This must be done when the OpenGL context is current
		mGpuTransfer->RegisterTexture(&mCpuObjects[i]);	
		
		//finally, add it to the circular buffer if the CPU circular buffer exists
		if(mCpuQueue[OUTQ])
			mCpuQueue[OUTQ]->Add(&mCpuObjects[i]);

	}		
	return true;
}

bool NTV2OglDPXReader::Deinit()
{		
	mCaptureContext->makeCurrent();	
	if(mGpuTransfer == NULL)
		return false;

	for( uint32_t i = 0; i < mNumOglObjects ; i++ ) {		
			mGpuTransfer->UnregisterTexture(&mOglObjects[i]);			
			mGpuTransfer->UnregisterTexture(&mCpuObjects[i]);			
	}	
	delete []  mOglObjects;	
	delete []  mCpuObjects;

	return true;
}




bool NTV2OglDPXReader::SetupThread()
{
	mAbortFlag = false;
	
	// Start Thread that will write file data to Device using NTV4 Sequencer
	mGpuTransfer->BeginTransfers();
	return true;
}
bool NTV2OglDPXReader::CleanupThread()
{
	mGpuTransfer->EndTransfers();
	return true;
}

bool NTV2OglDPXReader::Process()
{
		COglObject *gpuObject;
		CCpuObject *cpuObject;
		gpuObject = mGpuQueue[OUTQ]->StartProduceNextBuffer();
		if(gpuObject == NULL) return false;	
		if(mCpuQueue[OUTQ])
		{
			cpuObject = mCpuQueue[OUTQ]->StartProduceNextBuffer();
			if(cpuObject == NULL) return false;		
		}
		else
		{
			cpuObject = &mCpuObjects[0];
		}

		// Prepare for DMA transfer
		mGpuTransfer->BeforeRecordTransfer(gpuObject, cpuObject);	
		AJAFileIO file;	
		AJAStatus status = file.Open(mFileList[mCurrentFrameNumber].absoluteFilePath().toStdString(),eAJAReadOnly,eAJAUnbuffered);
		if ( status == AJA_STATUS_SUCCESS )
		{
			status = file.Seek(mImageOffset,eAJASeekCurrent);
			if ( status != AJA_STATUS_SUCCESS ) qDebug() << "Seek failed";
			uint64_t numBytes = file.Read((uint8_t*)cpuObject->GetVideoBuffer(),(uint32_t)cpuObject->GetSize());
			if ( numBytes != cpuObject->GetSize() ) qDebug() << "Read failed";
			status = file.Close();
////////////////////////////////
///////This is the part I would like to do in the shader
///////This format only needs bit swizzling. It would also be cool to handle our YCbCr DPX format but that is for later
///////It would be great if the shader converted to a float or half float did something to the image and then
///////actually pass a 10 bit RGB to the Output Thread.
			uint32_t pixel;
			uint32_t line;
			for ( line = 0; line < mHeight; line++ )
			{
				uint32_t* buffer = (uint32_t*)(cpuObject->GetVideoBuffer() + (line*mWidth*4));
				for ( pixel = 0; pixel < mWidth; pixel++ )
				{
					// Takes the 10 Bit RGB_DPX Big Endian and converts it to OpenGL friendly AJA_PixelFormat_ABGR8(NTV2_ABGR)
					// that feeds into alina's oglpassthruviewer.cpp
					// it would be nice to make a different viewer that not only showed the preview but did an operation on the
					// 10 bit RGB....then send it on to the ntv2ogloutput class....which can be modified to know what pixelformat
					// it will receive...i.e. 10 BIT RGB(AJA_PixelFormat_RGB10)
					uint8_t* pBits = (uint8_t*) buffer;
					uint32_t value = *buffer;
					*pBits++ = (value&0xFF);										  //Red
					*pBits++ = ((value & 0x3F00)>>6) + ((value & 0xC00000)>>22);	  //Green
					*pBits++ = ((value & 0xF0000000)>>28) + ((value&0x000F0000)>>12); //Blue
					*pBits++ = 0xFF;
					buffer ++;
				}
			}
//////////////////////////////////////////////////////////


		}
		mGpuTransfer->AfterRecordTransfer(gpuObject, cpuObject);
		mGpuQueue[OUTQ]->EndProduceNextBuffer();
		if(mCpuQueue[OUTQ])
		{
			mCpuQueue[OUTQ]->EndProduceNextBuffer();
		}

		mCurrentFrameNumber++;
		if (mCurrentFrameNumber == mNumFiles )
			mCurrentFrameNumber = 0;

	return true;
}

// note to bllbo - none if this information is actually used anywhere
void NTV2OglDPXReader::getFileInfo(QString fileNameStr)
{
	// Open video file to get parameters 
	QFile dpxFile(fileNameStr);
	DpxHdr dpxHeader;
	dpxFile.open(QIODevice::ReadOnly);
	dpxFile.read((char*)&(dpxHeader.GetHdr()),dpxHeader.GetHdrSize());

	mImageOffset			= (uint32_t)dpxHeader.get_fi_image_offset();
	mHeight					= (uint32_t)dpxHeader.get_ii_lines();
	mWidth					= (uint32_t)dpxHeader.get_ii_pixels();
	mFileImageSize			= (uint32_t)dpxHeader.get_ii_image_size();
	mFileFrameRate			= dpxHeader.get_film_frame_rate();

	mPixelFormat = AJA_PixelFormat_RGB_DPX;
	if ( dpxHeader.get_ie_descriptor() == 100 )
	{
		if ( dpxHeader.IsBigEndian())
		{
			mPixelFormat = AJA_PixelFormat_YCbCr_DPX;
		}
		else
		{
			// NOT REALLY SUPPORTED
			mPixelFormat = AJA_PixelFormat_YCbCr_DPX;
		}

	}
	else if ( dpxHeader.get_ie_descriptor() == 50 )
	{
		if ( dpxHeader.IsBigEndian())
		{
			mPixelFormat = AJA_PixelFormat_RGB_DPX;
		}
		else
		{
			mPixelFormat = AJA_PixelFormat_RGB_DPX_LE;
		}

	}
	dpxFile.close();
}

bool NTV2OglDPXReader::setupFiles()
{
	// For now, use QT for File I/O.
	QString dirStr(mDirectoryPath.c_str());
	QDir dir(dirStr);
	QStringList filters;
	filters << "*.dpx" << "*.DPX";
	dir.setNameFilters(filters);
	mFileList = dir.entryInfoList();
	mNumFiles = mFileList.size();

	return true;
}
