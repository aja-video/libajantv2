#ifndef NTV2OglDPXReader_H
#define NTV2OglDPXReader_H

#include <QtGui>
#include <QGLWidget>
#include <QtOpenGL>
#include <GL\glu.h>
#include "ajabase/common/public.h"
#include "ajabase/common/types.h"
#include "ajabase/common/buffer.h"
#include "ajabase/common/videotypes.h"
#include "time.h"
#include <ctime>

#include "gpustuff/include/oglTransfer.h"
#include "gpustuff/utility/videoprocessor.h"


//This class responsibility is to create the GPU objects, circular buffer and manage the capture into these objects
class NTV2OglDPXReader : public CVideoProcessor<COglObject>
{
	Q_OBJECT

	public:
	NTV2OglDPXReader(int deviceIndex, QGLWidget*		captureContext, IOglTransfer *gpuTransfer);
	~NTV2OglDPXReader();


protected:		
	virtual bool Init();
	virtual bool Deinit();
	//these are called in the thread
	virtual bool Process();
	virtual bool SetupThread();
	virtual bool CleanupThread();

	void getFileInfo(QString fileNameStr);
	bool setupFiles();

private:
	COglObject								*mOglObjects;
	CCpuObject								*mCpuObjects;
	int										mNumOglObjects;

	bool									mAbort;

	uint32_t								mHeight;
	uint32_t								mWidth;
	uint32_t								mImageOffset;
	uint32_t								mFileFrameRate;
	uint32_t								mFileImageSize;	

	std::string			mDirectoryPath;
	int32_t				mCurrentFrameNumber;
	int32_t				mNumFramesRead;
	AJA_PixelFormat		mPixelFormat;

	QFileInfoList		mFileList;
	uint32_t			mNumFiles;


	QGLWidget*			mCaptureContext;
};


#endif
