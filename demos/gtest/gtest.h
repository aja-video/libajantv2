#ifndef _GTEST_H
#define _GTEST_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2democommon.h"
#include "ntv2utils.h"
#include "ajabase/common/types.h"
#include "ajabase/common/videotypes.h"
#include "ajabase/system/debug.h"
#include "ajabase/system/thread.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"
#include <set>


typedef struct GTestConfig
{
	public:
		std::string			fDeviceSpec;		///< @brief	The AJA device to use
		NTV2Channel			fOutputChannel;		///< @brief	The output channel to use
		NTV2ACFrameRange	fOutputFrames;		///< @brief	Playout frame count or range
		NTV2PixelFormat		fPixelFormat;		///< @brief	The pixel format to use
		/**
			@brief	Constructs a default Player configuration.
		**/
		inline explicit	GTestConfig (const std::string & inDeviceSpecifier	= "0")
			:	fDeviceSpec			(inDeviceSpecifier),
				fOutputChannel		(NTV2_CHANNEL2),
				fOutputFrames		(7),
				fPixelFormat		(NTV2_FBF_8BIT_YCBCR)
		{
		}

}	GTestConfig;

class GTest
{
	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given configuration settings.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inConfig		Specifies the configuration parameters.
		**/
							GTest (const GTestConfig & inConfig);
		virtual				~GTest ();

		/**
			@brief	Initializes me and prepares me to Run.
		**/
		virtual AJAStatus	Init (void);

	//	Protected Instance Methods
	protected:
		/**
			@brief	Sets up everything I need for capturing and playing video.
		**/
		virtual AJAStatus	SetupVideo (void);

	//	Private Member Data
	private:
		GTestConfig			mConfig;				///< @brief	My configuration info
		CNTV2Card			mDevice;				///< @brief	My CNTV2Card instance
		NTV2DeviceID		mDeviceID;				///< @brief	Keep my device ID handy
		NTV2VideoFormat		mVideoFormat;			///< @brief	Format of video being ingested & played
		NTV2FormatDesc		mFormatDesc;			///< @brief	Describes raster images
		NTV2TaskMode		mSavedTaskMode;			///< @brief	For restoring prior state
		NTV2OutputDest		mOutputDest;			///< @brief	The desired output connector to use
		NTV2Buffer			mpHostVideoBuffer;		///< @brief My host video buffer

};	//	GTest

#endif	//	_GTEST_H
