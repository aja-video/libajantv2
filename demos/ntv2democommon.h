/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2democommon.h
	@brief		This file contains some structures, constants, classes and functions that are used in some of the demo applications.
				There is nothing magical about anything in this file. What's in here simply works well with the demos.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2DEMOCOMMON_H
#define _NTV2DEMOCOMMON_H

#include "ntv2rp188.h"
#include "ntv2publicinterface.h"
#include "ntv2card.h"
#include "ntv2utils.h"	//	for NTV2ACFrameRange
#include "ajaanc/includes/ancillarydata.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/common/videotypes.h"
#include "ajabase/common/circularbuffer.h"
#include "ajabase/system/debug.h"
#include "ajabase/system/info.h"
#include "ajabase/system/systemtime.h"	//	for AJATime
#include "ajabase/common/common.h"	//	for aja::strip & etc.
#include <algorithm>
#include <string>

//	Convenience macros for EZ logging:
#define	CAPFAIL(_expr_)		AJA_sERROR  (AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	CAPWARN(_expr_)		AJA_sWARNING(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	CAPDBG(_expr_)		AJA_sDEBUG	(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	CAPNOTE(_expr_)		AJA_sNOTICE	(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	CAPINFO(_expr_)		AJA_sINFO	(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)

#define	PLFAIL(_xpr_)		AJA_sERROR  (AJA_DebugUnit_DemoPlayout, AJAFUNC << ": " << _xpr_)
#define	PLWARN(_xpr_)		AJA_sWARNING(AJA_DebugUnit_DemoPlayout, AJAFUNC << ": " << _xpr_)
#define	PLNOTE(_xpr_)		AJA_sNOTICE	(AJA_DebugUnit_DemoPlayout, AJAFUNC << ": " << _xpr_)
#define	PLINFO(_xpr_)		AJA_sINFO	(AJA_DebugUnit_DemoPlayout, AJAFUNC << ": " << _xpr_)
#define	PLDBG(_xpr_)		AJA_sDEBUG	(AJA_DebugUnit_DemoPlayout, AJAFUNC << ": " << _xpr_)

#define	BURNFAIL(_expr_)	AJA_sERROR  (AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	BURNWARN(_expr_)	AJA_sWARNING(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	BURNDBG(_expr_)		AJA_sDEBUG	(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	BURNNOTE(_expr_)	AJA_sNOTICE	(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	BURNINFO(_expr_)	AJA_sINFO	(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)

#define NTV2_AUDIOSIZE_MAX	(401 * 1024)
#define NTV2_ANCSIZE_MAX	(0x2000)		//	8K


/**
	@brief	This structure encapsulates the video, audio and anc buffers used in the AutoCirculate demos.
			These demos use a fixed number (CIRCULAR_BUFFER_SIZE) of these buffers in an AJACircularBuffer,
			which greatly simplifies processing frames between producer and consumer threads.
**/
typedef struct
{
	uint32_t *		fVideoBuffer;			///< @brief	Pointer to host video buffer
	uint32_t *		fVideoBuffer2;			///< @brief	Pointer to an additional host video buffer, usually field 2
	uint32_t		fVideoBufferSize;		///< @brief	Size of host video buffer, in bytes
	uint32_t *		fAudioBuffer;			///< @brief	Pointer to host audio buffer
	uint32_t		fAudioBufferSize;		///< @brief	Size of host audio buffer, in bytes
	uint32_t *		fAncBuffer;				///< @brief	Pointer to ANC buffer
	uint32_t		fAncBufferSize;			///< @brief	Size of ANC buffer, in bytes
	uint32_t *		fAncF2Buffer;			///< @brief	Pointer to "Field 2" ANC buffer
	uint32_t		fAncF2BufferSize;		///< @brief	Size of "Field 2" ANC buffer, in bytes
	uint32_t		fAudioRecordSize;		///< @brief	For future use
	uint32_t		fAncRecordSize;			///< @brief	For future use
	RP188_STRUCT	fRP188Data;				///< @brief	For future use
	RP188_STRUCT	fRP188Data2;			///< @brief	For future use
	uint8_t *		fVideoBufferUnaligned;	///< @brief	For future use
	uint32_t		fFrameFlags;			///< @brief Frame data flags
} AVDataBuffer;


/**
	@brief	I encapsulate the video, audio and anc host buffers used in the AutoCirculate demos.
			I'm a more modern version of the AVDataBuffer.
**/
class AJAExport NTV2FrameData
{
	public:
		NTV2Buffer		fVideoBuffer;		///< @brief	Host video buffer
		NTV2Buffer		fVideoBuffer2;		///< @brief	Additional host video buffer, usually F2
		NTV2Buffer		fAudioBuffer;		///< @brief	Host audio buffer
		NTV2Buffer		fAncBuffer;			///< @brief	Host ancillary data buffer
		NTV2Buffer		fAncBuffer2;		///< @brief	Additional "F2" host anc buffer
		NTV2TimeCodes	fTimecodes;			///< @brief	Map of TC indexes to NTV2_RP188 values
		ULWord			fNumAudioBytes;		///< @brief	Actual number of captured audio bytes
		ULWord			fNumAncBytes;		///< @brief	Actual number of captured F1 anc bytes
		ULWord			fNumAnc2Bytes;		///< @brief	Actual number of captured F2 anc bytes
		uint32_t		fFrameFlags;		///< @brief Frame data flags
	public:
		explicit inline NTV2FrameData()
			:	fVideoBuffer	(0),
				fVideoBuffer2	(0),
				fAudioBuffer	(0),
				fAncBuffer		(0),
				fAncBuffer2		(0),
				fTimecodes		(),
				fNumAudioBytes	(0),
				fNumAncBytes	(0),
				fNumAnc2Bytes	(0),
				fFrameFlags(0)	{}

		//	Inquiry Methods
		inline NTV2Buffer &	VideoBuffer (void)							{return fVideoBuffer;}
		inline ULWord		VideoBufferSize (void) const				{return fVideoBuffer.GetByteCount();}

		inline NTV2Buffer &	AudioBuffer (void)							{return fAudioBuffer;}
		inline ULWord		AudioBufferSize (void) const				{return fAudioBuffer.GetByteCount();}
		inline ULWord		NumCapturedAudioBytes (void) const			{return fNumAudioBytes;}

		inline NTV2Buffer &	AncBuffer (void)							{return fAncBuffer;}
		inline ULWord		AncBufferSize (void) const					{return fAncBuffer.GetByteCount();}
		inline ULWord		NumCapturedAncBytes (void) const			{return fNumAncBytes;}

		inline NTV2Buffer &	AncBuffer2 (void)							{return fAncBuffer2;}
		inline ULWord		AncBuffer2Size (void) const					{return fAncBuffer2.GetByteCount();}
		inline ULWord		NumCapturedAnc2Bytes (void) const			{return fNumAnc2Bytes;}

		inline NTV2Buffer &	VideoBuffer2 (void)							{return fVideoBuffer2;}
		inline ULWord		VideoBufferSize2 (void) const				{return fVideoBuffer2.GetByteCount();}

		inline bool			IsNULL (void) const							{return fVideoBuffer.IsNULL() && fVideoBuffer2.IsNULL()
																			&& fAudioBuffer.IsNULL() && fAncBuffer.IsNULL()
																			&& fAncBuffer2.IsNULL();}
		inline bool	HasTimecode (const NTV2TCIndex inTCNdx) const		{return fTimecodes.find(inTCNdx) != fTimecodes.end();}
		NTV2_RP188	Timecode (const NTV2TCIndex inTCNdx) const;
		inline bool	HasValidTimecode (const NTV2TCIndex inTCNdx) const	{return Timecode(inTCNdx).IsValid();}

		//	Modifier Methods
		inline void		ZeroBuffers (void)		{	if (fVideoBuffer)
														fVideoBuffer.Fill(ULWord(0));
													if (fVideoBuffer2)
														fVideoBuffer2.Fill(ULWord(0));
													if (fAudioBuffer)
														fAudioBuffer.Fill(ULWord(0));
													if (fAncBuffer)
														fAncBuffer.Fill(ULWord(0));
													if (fAncBuffer2)
														fAncBuffer2.Fill(ULWord(0));
													fNumAudioBytes = fNumAncBytes = fNumAnc2Bytes = 0;
												}
		bool			LockAll					(CNTV2Card & inDevice);
		bool			UnlockAll				(CNTV2Card & inDevice);

		bool			Reset (void)			{return fVideoBuffer.Allocate(0) && fVideoBuffer2.Allocate(0)
														&& fAudioBuffer.Allocate(0) && fAncBuffer.Allocate(0)
														&& fAncBuffer2.Allocate(0);}
};	//	NTV2FrameData

typedef std::vector<NTV2FrameData>			NTV2FrameDataArray;				///< @brief A vector of NTV2FrameData elements
typedef NTV2FrameDataArray::iterator		NTV2FrameDataArrayIter;			///< @brief Handy non-const iterator
typedef NTV2FrameDataArray::const_iterator	NTV2FrameDataArrayConstIter;	///< @brief Handy const iterator
typedef	AJACircularBuffer<NTV2FrameData*>	FrameDataRingBuffer;			///< @brief	Buffer ring of NTV2FrameData's



static const size_t CIRCULAR_BUFFER_SIZE	(10);	///< @brief	Number of NTV2FrameData's in our ring
static const ULWord	kDemoAppSignature		NTV2_FOURCC('D','E','M','O');


/**
	@brief	A handy class that makes it easy to "bounce" an unsigned integer value between a minimum and maximum value
			using sequential calls to its Next method.
**/
template <typename T> class Bouncer
{
	public:
		inline Bouncer (const T inUpperLimit, const T inLowerLimit = T(0), const T inStartValue = T(0), const bool inStartAscend = true)
			:	mMin		(inLowerLimit),
				mMax		(inUpperLimit),
				mValue		(inStartValue),
				mIncrement	(T(1)),
				mAscend		(inStartAscend)
		{
			if (mMin > mMax)
				std::swap (mMin, mMax);
			else if (mMin == mMax)
				mMax = mMin + mIncrement;
			if (mValue < mMin)
			{
				mValue = mMin;
				mAscend = true;
			}
			if (mValue > mMax)
			{
				mValue = mMax;
				mAscend = false;
			}
		}

		inline T	Next (void)
		{
			if (mAscend)
			{
				if (mValue < mMax)
					mValue += mIncrement;
				else
					mAscend = false;
			}
			else
			{
				if (mValue > mMin)
					mValue -= mIncrement;
				else
					mAscend = true;
			}
			return mValue;
		}

		inline void	SetIncrement (const T inValue)	{mIncrement = inValue;}
		inline T	Value (void) const	{return mValue;}

	private:
		T		mMin, mMax, mValue, mIncrement;
		bool	mAscend;

};	//	Bouncer


typedef enum _NTV2VideoFormatKind
{
	VIDEO_FORMATS_NONE			= 0,
	VIDEO_FORMATS_SDHD			= 1,
	VIDEO_FORMATS_4KUHD			= 2,
	VIDEO_FORMATS_8KUHD2		= 4,
	VIDEO_FORMATS_ALL_UHD4K8K	= (VIDEO_FORMATS_4KUHD | VIDEO_FORMATS_8KUHD2),
	VIDEO_FORMATS_ALL			= (VIDEO_FORMATS_SDHD | VIDEO_FORMATS_4KUHD | VIDEO_FORMATS_8KUHD2)
#if !defined(NTV2_DEPRECATE_17_5)
	//	Deprecated old ones:
	,VIDEO_FORMATS_NON_4KUHD	= VIDEO_FORMATS_SDHD
	,VIDEO_FORMATS_UHD2			= VIDEO_FORMATS_8KUHD2
	,BOTH_VIDEO_FORMATS			= VIDEO_FORMATS_ALL
	,NON_UHD_VIDEO_FORMATS		= VIDEO_FORMATS_SDHD
	,UHD_VIDEO_FORMATS			= VIDEO_FORMATS_4KUHD
#endif	//	!defined(NTV2_DEPRECATE_17_5)
} NTV2VideoFormatKind;

typedef ULWord NTV2VideoFormatKinds;


typedef enum _NTV2PixelFormatKind
{
	PIXEL_FORMATS_NONE		= 0,
	PIXEL_FORMATS_RGB		= 1,
	PIXEL_FORMATS_PLANAR	= 2,
	PIXEL_FORMATS_RAW		= 4,
	PIXEL_FORMATS_PACKED	= 8,
	PIXEL_FORMATS_ALPHA		= 16,
	PIXEL_FORMATS_NO_RAW	= (PIXEL_FORMATS_RGB | PIXEL_FORMATS_PLANAR | PIXEL_FORMATS_PACKED | PIXEL_FORMATS_ALPHA),
	PIXEL_FORMATS_ALL		= (PIXEL_FORMATS_RGB | PIXEL_FORMATS_PLANAR | PIXEL_FORMATS_RAW | PIXEL_FORMATS_PACKED | PIXEL_FORMATS_ALPHA)
} NTV2PixelFormatKind;

typedef ULWord NTV2PixelFormatKinds;


typedef enum _NTV2TCIndexKinds
{
	TC_INDEXES_NONE			= 0,
	TC_INDEXES_SDI			= 1,
	TC_INDEXES_ANALOG		= 2,
	TC_INDEXES_ATCLTC		= 4,
	TC_INDEXES_VITC1		= 8,
	TC_INDEXES_VITC2		= 16,
	TC_INDEXES_NO_ANALOG	= (TC_INDEXES_SDI | TC_INDEXES_ATCLTC | TC_INDEXES_VITC1 | TC_INDEXES_VITC2),
	TC_INDEXES_ALL			= (TC_INDEXES_NO_ANALOG | TC_INDEXES_ANALOG )
} NTV2TCIndexKinds;


/**
	@brief	This class is used to configure an NTV2Capture instance.
**/
class AJAExport CaptureConfig
{
	public:
		std::string			fDeviceSpec;		///< @brief	The AJA device to use
		std::string			fAncDataFilePath;	///< @brief	Optional path to Anc binary data file
		NTV2Channel			fInputChannel;		///< @brief	The device channel to use
		NTV2InputSource		fInputSource;		///< @brief	The device input connector to use
		NTV2ACFrameRange	fFrames;			///< @brief	AutoCirculate frame count or range
		NTV2PixelFormat		fPixelFormat;		///< @brief	Pixel format to use
		UWord				fNumAudioLinks;		///< @brief	Number of audio links to capture
		bool				fDoABConversion;	///< @brief	If true, do level-A/B conversion;  otherwise don't
		bool				fDoMultiFormat;		///< @brief	If true, use multi-format/multi-channel mode, if device supports it; otherwise normal mode
		bool				fWithAnc;			///< @brief	If true, also capture Anc
		bool				fWithAudio;			///< @brief	If true, also capture Audio
		bool				fDoTSIRouting;		///< @brief	If true, do TSI routing; otherwise squares

		/**
			@brief	Constructs a default NTV2Capture configuration.
		**/
		inline explicit	CaptureConfig (const std::string & inDeviceSpec	= "0")
			:	fDeviceSpec			(inDeviceSpec),
				fAncDataFilePath	(),
				fInputChannel		(NTV2_CHANNEL_INVALID),
				fInputSource		(NTV2_INPUTSOURCE_INVALID),
				fFrames				(7),
				fPixelFormat		(NTV2_FBF_8BIT_YCBCR),
				fNumAudioLinks		(1),
				fDoABConversion		(false),
				fDoMultiFormat		(false),
				fWithAnc			(false),
				fWithAudio			(true),
				fDoTSIRouting		(true)
		{
		}

		AJALabelValuePairs	Get (const bool inCompact = false) const;

};	//	CaptureConfig

AJAExport std::ostream &	operator << (std::ostream & ioStrm, const CaptureConfig & inObj);


/**
	@brief	Configures an NTV2Player instance.
**/
typedef struct PlayerConfig
{
	public:
		std::string			fDeviceSpec;		///< @brief	The AJA device to use
		std::string			fAncDataFilePath;	///< @brief	Optional path to Anc binary data file to playout
		NTV2Channel			fOutputChannel;		///< @brief	The device channel to use
		NTV2OutputDest		fOutputDest;		///< @brief	The desired output connector to use
		NTV2ACFrameRange	fFrames;			///< @brief	AutoCirculate frame count or range
		NTV2PixelFormat		fPixelFormat;		///< @brief	The pixel format to use
		NTV2VideoFormat		fVideoFormat;		///< @brief	The video format to use
		NTV2VANCMode		fVancMode;			///< @brief	VANC mode to use
		UWord				fNumAudioLinks;		///< @brief	The number of audio systems to control for multi-link audio (4K/8K)
		bool				fDoMultiFormat;		///< @brief	If true, enable device-sharing;  otherwise take exclusive control of device
		bool				fSuppressAudio;		///< @brief	If true, suppress audio;  otherwise generate & xfer audio tone
		bool				fSuppressVideo;		///< @brief	If true, suppress video;  otherwise generate & xfer test patterns
		bool				fTransmitLTC;		///< @brief	If true, embed LTC;  otherwise embed VITC
		bool				fDoABConversion;	///< @brief	If true, do level-A/B conversion;  otherwise don't
		bool				fDoHDMIOutput;		///< @brief	If true, enable HDMI output;  otherwise, disable HDMI output
		bool				fDoTsiRouting;		///< @brief	If true, enable TSI routing; otherwise route for square division (4K/8K)
		bool				fDoRGBOnWire;		///< @brief	If true, produce RGB on the wire; otherwise output YUV
		bool				fDoLinkGrouping;	///< @brief	If true, enables 6/12G output mode on IoX3/Kona5 (4K/8K)

		/**
			@brief	Constructs a default Player configuration.
		**/
		inline explicit	PlayerConfig (const std::string & inDeviceSpecifier	= "0")
			:	fDeviceSpec			(inDeviceSpecifier),
				fAncDataFilePath	(),
				fOutputChannel		(NTV2_CHANNEL1),
				fOutputDest			(NTV2_OUTPUTDESTINATION_SDI2),
				fFrames				(7),
				fPixelFormat		(NTV2_FBF_8BIT_YCBCR),
				fVideoFormat		(NTV2_FORMAT_1080i_5994),
				fVancMode			(NTV2_VANCMODE_OFF),
				fNumAudioLinks		(1),
				fDoMultiFormat		(false),
				fSuppressAudio		(false),
				fSuppressVideo		(false),
				fTransmitLTC		(false),
				fDoABConversion		(false),
				fDoHDMIOutput		(false),
				fDoTsiRouting		(false),
				fDoRGBOnWire		(false),
				fDoLinkGrouping		(false)
		{
		}

		inline bool	WithAudio(void) const	{return !fSuppressAudio  &&  fNumAudioLinks > 0;}	///< @return	True if playing audio, false if not.
		inline bool	WithVideo(void) const	{return !fSuppressVideo;}	///< @return	True if playing video, false if not.

		/**
			@brief		Renders a human-readable representation of me.
			@param[in]	inCompact	If true, setting values are printed in a more compact form. Defaults to false.
			@return		A list of label/value pairs.
		**/
		AJALabelValuePairs Get (const bool inCompact = false) const;

}	PlayerConfig;

AJAExport std::ostream &	operator << (std::ostream & ioStrm, const PlayerConfig & inObj);

/**
	@brief	Configures an NTV2Burn or NTV2FieldBurn instance.
**/
typedef struct BurnConfig
{
	public:
		std::string			fDeviceSpec;		///< @brief	The AJA device to use
		std::string			fDeviceSpec2;		///< @brief	Second AJA device to use (Burn4KQuadrant or BurnBoardToBoard only)
		NTV2Channel			fInputChannel;		///< @brief	The input channel to use
		NTV2Channel			fOutputChannel;		///< @brief	The output channel to use
		NTV2InputSource		fInputSource;		///< @brief	The device input connector to use
		NTV2OutputDest		fOutputDest;		///< @brief	The device output connector to use (NTV2_OUTPUTDESTINATION_INVALID means unspecified)
		NTV2ACFrameRange	fInputFrames;		///< @brief	Ingest frame count or range
		NTV2ACFrameRange	fOutputFrames;		///< @brief	Playout frame count or range
		NTV2PixelFormat		fPixelFormat;		///< @brief	The pixel format to use
		NTV2TCIndex			fTimecodeSource;	///< @brief	Timecode source to use
		bool				fDoMultiFormat;		///< @brief	If true, enables device-sharing;  otherwise takes exclusive control of the device.
		bool				fSuppressAudio;		///< @brief	If true, suppress audio;  otherwise include audio
		bool				fSuppressVideo;		///< @brief	If true, suppress video;  otherwise include video
		bool				fIsFieldMode;		///< @brief	True if Field Mode, otherwise Frame Mode
		bool				fWithAnc;			///< @brief	If true, capture & play anc data (LLBurn). Defaults to false.
		bool				fWithTallFrames;	///< @brief	If true && fWithAnc, use "taller" VANC mode for anc. Defaults to false.
		bool				fWithHanc;			///< @brief	If true, capture & play HANC data, including audio (LLBurn). Defaults to false.
		bool				fVerbose;			///< @brief	If true, emit explanatory messages to stdout/stderr. Defaults to false.

		/**
			@brief	Constructs a default Player configuration.
		**/
		inline explicit	BurnConfig (const std::string & inDeviceSpecifier	= "0")
			:	fDeviceSpec			(inDeviceSpecifier),
				fDeviceSpec2		(),
				fInputChannel		(NTV2_CHANNEL1),
				fOutputChannel		(NTV2_CHANNEL3),
				fInputSource		(NTV2_INPUTSOURCE_SDI1),
				fOutputDest			(NTV2_OUTPUTDESTINATION_INVALID),
				fInputFrames		(7),
				fOutputFrames		(7),
				fPixelFormat		(NTV2_FBF_8BIT_YCBCR),
				fTimecodeSource		(NTV2_TCINDEX_SDI1),
				fDoMultiFormat		(false),
				fSuppressAudio		(false),
				fSuppressVideo		(false),
				fIsFieldMode		(false),
				fWithAnc			(false),
				fWithTallFrames		(false),
				fWithHanc			(false),
				fVerbose			(false)
		{
		}

		inline bool	WithAudio(void) const		{return !fSuppressAudio;}	///< @return	True if streaming audio, false if not.
		inline bool	WithVideo(void) const		{return !fSuppressVideo;}	///< @return	True if streaming video, false if not.
		inline bool	WithAnc(void) const			{return fWithAnc;}			///< @return	True if streaming anc data, false if not.
		inline bool WithTallVANC(void) const	{return fWithTallFrames;}	///< @return	True if using taller VANC mode, otherwise false.
		inline bool	WithHanc(void) const		{return fWithHanc;}			///< @return	True if streaming HANC, false if not.
		inline bool WithTimecode(void) const	{return NTV2_IS_VALID_TIMECODE_INDEX(fTimecodeSource);}	///< @return	True if valid TC source
		inline bool FieldMode(void) const		{return fIsFieldMode;}		///< @return	True if field mode, otherwise false.
		inline bool OutputSpecified(void) const	{return NTV2_IS_VALID_OUTPUT_DEST(fOutputDest);}	///< @return	True if output destination was specified
		inline bool	IsVerbose(void) const		{return fVerbose;}			///< @return	True if in verbose mode, otherwise false.
		inline std::string	ISrcStr(void) const	{return ::NTV2InputSourceToString(fInputSource, true);}
		inline NTV2Channel	ISrcCh(void) const	{return ::NTV2InputSourceToChannel(fInputSource);}
		inline bool	ISrcIsSDI(void) const		{return NTV2_INPUT_SOURCE_IS_SDI(fInputSource);}
		inline std::string	ODstStr(void) const	{return ::NTV2OutputDestinationToString(fOutputDest, true);}
		inline NTV2Channel	ODstCh(void) const	{return ::NTV2OutputDestinationToChannel(fOutputDest);}
		inline std::string	IChStr(void) const	{std::ostringstream oss; oss << "Ch" << int(fInputChannel); return oss.str();}
		inline std::string	OChStr(void) const	{std::ostringstream oss; oss << "Ch" << int(fOutputChannel); return oss.str();}
		inline bool	ODstIsSDI(void) const		{return NTV2_OUTPUT_DEST_IS_SDI(fOutputDest);}

		/**
			@brief		Renders a human-readable representation of me.
			@param[in]	inCompact	If true, setting values are printed in a more compact form. Defaults to false.
			@return		A list of label/value pairs.
		**/
		AJALabelValuePairs Get (const bool inCompact = false) const;

}	BurnConfig;

/**
	@brief		Renders a human-readable representation of a BurnConfig into an output stream.
	@param		strm	The output stream.
	@param[in]	inObj	The configuration to be rendered into the output stream.
	@return		A reference to the specified output stream.
**/
inline std::ostream &	operator << (std::ostream & strm, const BurnConfig & inObj)	{return strm << AJASystemInfo::ToString(inObj.Get());}



/**
	@brief	A set of common convenience functions used by the NTV2 \ref demoapps.
			Most are used for converting a command line argument into ::NTV2VideoFormat,
			::NTV2FrameBufferFormat, etc. types.
**/
class AJAExport CNTV2DemoCommon
{
	public:
	/**
		@name	Device Functions
	**/
	///@{
		/**
			@param[in]	inDeviceSpec	A string containing a decimal index number, device serial number, or a device model name.
			@return		True if the specified device exists and can be opened.
		**/
		static bool							IsValidDevice (const std::string & inDeviceSpec);
	///@}

	/**
		@name	Video Format Functions
	**/
	///@{
		/**
			@param[in]	inKinds		Specifies the types of video formats returned. Defaults to non-4K/UHD formats.
			@return		The supported ::NTV2VideoFormatSet.
		**/
		static NTV2VideoFormatSet			GetSupportedVideoFormats (const NTV2VideoFormatKinds inKinds = VIDEO_FORMATS_SDHD);

		/**
			@param[in]	inKinds		Specifies the types of video formats returned. Defaults to non-4K/UHD formats.
			@param[in]	inDevSpec	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
									returns those video formats that are supported by the device.
			@return		A string that can be printed to show the supported video formats.
			@note		These video format strings are mere conveniences for specifying video formats in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetVideoFormatStrings (const NTV2VideoFormatKinds inKinds = VIDEO_FORMATS_SDHD,
																	const std::string inDevSpec = std::string());

		/**
			@brief	Returns the ::NTV2VideoFormat that matches the given string.
			@param[in]	inStr		Specifies the string to be converted to an ::NTV2VideoFormat.
			@param[in]	inKinds		Specifies which video format type is expected in "inStr", whether non-4K/UHD (the default),
									exclusively 4K/UHD, or both/all.
			@param[in]	inDevSpec	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
									returns a valid video format only if supported by the device.
			@return		The given string converted to an ::NTV2VideoFormat, or ::NTV2_FORMAT_UNKNOWN if there's no match.
		**/
		static NTV2VideoFormat				GetVideoFormatFromString (const std::string & inStr,
																		const NTV2VideoFormatKinds inKinds	= VIDEO_FORMATS_SDHD,
																		const std::string & inDevSpec		= std::string());

		/**
			@brief		Given a video format, if all 4 inputs are the same and promotable to 4K, this function does the promotion.
			@param		inOutVideoFormat	On entry, specifies the wire format;  on exit, receives the 4K video format.
			@return		True if successful;  otherwise false.
		**/
		static bool							Get4KInputFormat (NTV2VideoFormat & inOutVideoFormat);
		
		/**
			@brief		Given a video format, if all 4 inputs are the same and promotable to 8K, this function does the promotion.
			@param		inOutVideoFormat	On entry, specifies the wire format;  on exit, receives the 4K video format.
			@return		True if successful;  otherwise false.
		**/
		static bool							Get8KInputFormat (NTV2VideoFormat & inOutVideoFormat);
	///@}

	/**
		@name	Pixel Format Functions
	**/
	///@{
		/**
			@param[in]	inKinds		Specifies the types of pixel formats returned. Defaults to all formats.
			@return		The supported ::NTV2FrameBufferFormatSet.
		**/
		static NTV2PixelFormats				GetSupportedPixelFormats (const NTV2PixelFormatKinds inKinds = PIXEL_FORMATS_ALL);

		/**
			@param[in]	inKinds		Specifies the types of pixel formats returned. Defaults to all formats.
			@param[in]	inDevSpec	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
									returns those pixel formats that are supported by the device.
			@return		A string that can be printed to show the available pixel formats (or those that are supported by a given device).
			@note		These pixel format strings are mere conveniences for specifying pixel formats in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetPixelFormatStrings (const NTV2PixelFormatKinds inKinds = PIXEL_FORMATS_ALL,
																	const std::string inDevSpec = std::string());

		/**
			@brief	Returns the ::NTV2PixelFormat that matches the given string.
			@param[in]	inStr		Specifies the string to be converted to an ::NTV2PixelFormat.
			@param[in]	inKinds		Specifies the types of pixel formats returned. Defaults to all formats.
			@param[in]	inDevSpec	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
									returns a valid pixel format only if supported by the device.
			@return		An ::NTV2PixelFormat, or ::NTV2_FBF_INVALID if there's no match.
		**/
		static NTV2PixelFormat				GetPixelFormatFromString (const std::string & inStr,
																		const NTV2PixelFormatKinds inKinds = PIXEL_FORMATS_ALL,
																		const std::string inDevSpec = std::string());

		/**
			@return		The equivalent ::AJA_PixelFormat for the given ::NTV2FrameBufferFormat.
			@param[in]	inFormat	Specifies the ::NTV2FrameBufferFormat to be converted into an equivalent ::AJA_PixelFormat.
		**/
		static AJA_PixelFormat				GetAJAPixelFormat (const NTV2PixelFormat inFormat);
	///@}

	/**
		@name	Input Source Functions
	**/
	///@{
		/**
			@param[in]	inKinds		Specifies the types of input sources returned. Defaults to all sources.
			@return		The supported ::NTV2InputSourceSet.
		**/
		static const NTV2InputSourceSet		GetSupportedInputSources (const NTV2IOKinds inKinds = NTV2_IOKINDS_ALL);

		/**
			@param[in]	inKinds		Specifies the types of input sources returned. Defaults to all sources.
			@param[in]	inDevSpec	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
									returns those input sources that are supported by the device.
			@return		A string that can be printed to show the available input sources (or those that are supported by a given device).
			@note		These input source strings are mere conveniences for specifying input sources in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetInputSourceStrings (const NTV2IOKinds inKinds = NTV2_IOKINDS_ALL,
																	const std::string inDevSpec = std::string ());

		/**
			@brief		Returns the ::NTV2InputSource that matches the given string.
			@param[in]	inStr		Specifies the string to be converted to an ::NTV2InputSource.
			@param[in]	inKinds		Specifies the types of input sources returned. Defaults to all sources.
			@param[in]	inDevSpec	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
									returns a valid input source only if supported by the device.
			@return		The given string converted to an ::NTV2InputSource, or ::NTV2_INPUTSOURCE_INVALID if there's no match.
		**/
		static NTV2InputSource				GetInputSourceFromString (const std::string & inStr,
																		const NTV2IOKinds inKinds = NTV2_IOKINDS_ALL,
																		const std::string inDevSpec = std::string());
	///@}

	/**
		@name	Output Destination Functions
	**/
	///@{
		/**
			@param[in]	inKinds		Specifies the types of output destinations returned. Defaults to all types.
			@return		The supported ::NTV2OutputDestinations.
		**/
		static const NTV2OutputDestinations	GetSupportedOutputDestinations (const NTV2IOKinds inKinds);

		/**
			@param[in]	inKinds		Specifies the types of output destinations returned. Defaults to all types.
			@param[in]	inDevSpec	An optional device specifier. If non-empty, and resolves to a valid device, the returned
									string will only contain output destination values that are compatible with that device.
			@return		A string that can be printed to show the available output destinations (or those that are supported by a given device).
			@note		These output destination strings are mere conveniences for specifying output destinations in the
						command-line-based demo apps, and are subject to change without notice. They are not intended to
						be canonical in any way.
		**/
		static std::string					GetOutputDestinationStrings (const NTV2IOKinds inKinds, const std::string inDevSpec = std::string ());

		/**
			@brief		Returns the ::NTV2OutputDestination that matches the given string.
			@param[in]	inStr		Specifies the string to be converted to an ::NTV2OutputDestination.
			@param[in]	inKinds		Specifies the types of output destinations returned. Defaults to all types.
			@param[in]	inDevSpec	An optional device specifier. If non-empty, and resolves to a valid device,
									the returned value, if valid, will be compatible with that device.
			@return		The given string converted to an ::NTV2OutputDestination, or ::NTV2_OUTPUTDESTINATION_INVALID if there's no match.
		**/
		static NTV2OutputDestination		GetOutputDestinationFromString (const std::string & inStr,
																			const NTV2IOKinds inKinds = NTV2_IOKINDS_ALL,
																			const std::string inDevSpec = std::string());
	///@}

	/**
		@name	Timecode Functions
	**/
	///@{
		/**
			@param[in]	inKinds				Specifies the types of timecode indexes returned. Defaults to all indexes.
			@return		The supported ::NTV2TCIndexes set.
		**/
		static const NTV2TCIndexes			GetSupportedTCIndexes (const NTV2TCIndexKinds inKinds);

		/**
			@param[in]	inKinds				Specifies the types of timecode indexes returned. Defaults to all indexes.
			@param[in]	inDeviceSpecifier	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
											returns a valid timecode index only if supported by the device.
			@param[in]	inIsInputOnly		Optionally specifies if intended for timecode input (capture).
											Defaults to 'true'. Specify 'false' to obtain the list of timecode indexes
											that are valid for the given device for either input (capture) or output
											(playout).
			@return		A string that can be printed to show the available timecode indexes (or those that are supported by a given device).
			@note		These timecode index strings are mere conveniences for specifying timecode indexes in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetTCIndexStrings (const NTV2TCIndexKinds inKinds = TC_INDEXES_ALL,
																const std::string inDeviceSpecifier = std::string(),
																const bool inIsInputOnly = true);

		/**
			@brief		Returns the ::NTV2TCIndex that matches the given string.
			@param[in]	inStr		Specifies the string to be converted to an ::NTV2TCIndex.
			@param[in]	inKinds		Optionally specifies the timecode types to filter for.
			@param[in]	inDevSpec	Optionally specifies the device specification.
			@return		The given string converted to an ::NTV2TCIndex, or ::NTV2_TCINDEX_INVALID if there's no match.
		**/
		static NTV2TCIndex					GetTCIndexFromString (const std::string & inStr,
																const NTV2TCIndexKinds inKinds = TC_INDEXES_ALL,
																const std::string inDevSpec = std::string());
	///@}

	/**
		@name	Audio System Functions
	**/
	///@{
		/**
			@param[in]	inDeviceSpecifier	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
											returns the audio systems that are compatible with that device.
			@return		A string that can be printed to show the available audio systems that are supported by a given device.
			@note		These audio system strings are mere conveniences for specifying audio systems in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetAudioSystemStrings (const std::string inDeviceSpecifier = std::string ());

		/**
			@brief	Returns the ::NTV2AudioSystem that matches the given string.
			@param[in]	inStr	Specifies the string to be converted to an ::NTV2AudioSystem.
			@return		The given string converted to an ::NTV2AudioSystem, or ::NTV2_AUDIOSYSTEM_INVALID if there's no match.
		**/
		static NTV2AudioSystem				GetAudioSystemFromString (const std::string & inStr);
	///@}

	/**
		@name	Test Pattern Functions
	**/
	///@{
		/**
			@return		A string that can be printed to show the available test pattern and color identifiers.
			@note		These test pattern strings are mere conveniences for specifying test patterns in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetTestPatternStrings (void);

		/**
			@param[in]	inStr	Specifies the string to be converted to a valid test pattern or color name.
			@return		The test pattern or color name that best matches the given string, or an empty string if invalid.
		**/
		static std::string					GetTestPatternNameFromString (const std::string & inStr);
	///@}

	/**
		@name	VANC Mode Functions
	**/
	///@{
		/**
			@return		A string that can be printed to show the available VANC mode identifiers.
			@note		These identifiers are mere conveniences for specifying VANC modes in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetVANCModeStrings (void);

		/**
			@param[in]	inStr	Specifies the string to be converted to a NTV2VANCMode.
			@return		The NTV2VANCMode that best matches the given string, or an empty string if invalid.
		**/
		static NTV2VANCMode					GetVANCModeFromString (const std::string & inStr);
	///@}

	/**
		@name	Routing Functions
	**/
	///@{
		/**
			@brief		Answers with the crosspoint connections needed to implement the given capture configuration.
			@param[in]	inConfig		Specifies the CaptureConfig to route for.
			@param[in]	isInputRGB		Optionally specifies if the input is RGB. Defaults to false (YUV).
			@param[out]	outConnections	Receives the crosspoint connection set.
		**/
		static bool							GetInputRouting (NTV2XptConnections & outConnections,
															const CaptureConfig & inConfig,
															const bool isInputRGB = false);

		/**
			@brief		Answers with the crosspoint connections needed to implement the given 4K/UHD capture configuration.
			@param[in]	inConfig		Specifies the CaptureConfig to route for.
			@param[in]	inDevID			Optionally specifies the NTV2DeviceID.
			@param[in]	isInputRGB		Optionally specifies if the input is RGB. Defaults to false (YUV).
			@param[out]	outConnections	Receives the crosspoint connection set.
		**/
		static bool							GetInputRouting4K (	NTV2XptConnections & outConnections,
																const CaptureConfig & inConfig,
																const NTV2DeviceID inDevID = DEVICE_ID_INVALID,
																const bool isInputRGB = false);

		/**
			@brief		Answers with the crosspoint connections needed to implement the given 8K/UHD2 capture configuration.
			@param[out]	outConnections	Receives the crosspoint connection set.
			@param[in]	inConfig		Specifies the CaptureConfig to route for.
			@param[in]	inVidFormat		Specifies the NTV2VideoFormat.
			@param[in]	inDevID			Optionally specifies the NTV2DeviceID.
			@param[in]	isInputRGB		Optionally specifies if the input is RGB. Defaults to false (YUV).
		**/
		static bool							GetInputRouting8K (	NTV2XptConnections & outConnections,
																const CaptureConfig & inConfig,
																const NTV2VideoFormat inVidFormat,
																const NTV2DeviceID inDevID = DEVICE_ID_INVALID,
																const bool isInputRGB = false);
	///@}

	/**
		@name	Miscellaneous Functions
	**/
	///@{
		/**
			@brief	Returns the given string after converting it to lower case.
			@param[in]	inStr	Specifies the string to be converted to lower case.
			@return		The given string converted to lower-case.
			@note		Only works with ASCII characters!
		**/
		static std::string					ToLower (const std::string & inStr);

		/**
			@param[in]	inStr	Specifies the string to be stripped.
			@return		The given string after stripping all spaces, periods, and "00"s.
			@note		Only works with ASCII character strings!
		**/
		static std::string					StripFormatString (const std::string & inStr);

		/**
			@brief	Returns the character that represents the last key that was pressed on the keyboard
					without waiting for Enter or Return to be pressed.
		**/
		static char							ReadCharacterPress (void);

		/**
			@brief	Prompts the user (via stdout) to press the Return or Enter key, then waits for it to happen.
		**/
		static void							WaitForEnterKeyPress (void);

		/**
		@return		The equivalent TimecodeFormat for a given NTV2FrameRate.
		@param[in]	inFrameRate		Specifies the NTV2FrameRate to be converted into an equivalent TimecodeFormat.
		**/
		static TimecodeFormat				NTV2FrameRate2TimecodeFormat(const NTV2FrameRate inFrameRate);

		/**
			@return		The equivalent AJA_FrameRate for the given NTV2FrameRate.
			@param[in]	inFrameRate	Specifies the NTV2FrameRate to be converted into an equivalent AJA_FrameRate.
		**/
		static AJA_FrameRate				GetAJAFrameRate (const NTV2FrameRate inFrameRate);

		/**
			@return		A pointer to a 'C' string containing the name of the AJA NTV2 demonstration application global mutex.
		**/
		static const char *					GetGlobalMutexName (void);

		/**
			@return		The TSIMuxes to use given the first FrameStore on the device and a count.
			@param[in]	inDevice		Specifies the device being used.
			@param[in]	in1stFrameStore	Specifies the first FrameStore of interest.
			@param[in]	inCount			Specifies the number of Muxes.
		**/
		static NTV2ChannelList				GetTSIMuxesForFrameStore (CNTV2Card & inDevice, const NTV2Channel in1stFrameStore, const UWord inCount);

		/**
			@brief		Configures capture audio systems.
			@param[in]	inDevice		Specifies the device to configure.
			@param[in]	inConfig    	Specifies the capture configuration (primarily for fInputSource).
			@param[in]	inAudioSystems	Specifies the audio systems to configure.
			@return		True if successful;  otherwise false.
		**/
		static bool							ConfigureAudioSystems (CNTV2Card & inDevice, const CaptureConfig & inConfig, const NTV2AudioSystemSet inAudioSystems);

		static size_t						SetDefaultPageSize (void);
	///@}

	typedef NTV2ACFrameRange	ACFrameRange;	///< @deprecated	Starting in SDK 17.0, use NTV2ACFrameRange from now on

	typedef struct poptOption	PoptOpts;
	class AJAExport Popt
	{
		public:
			Popt (const int inArgc, const char ** pArgs, const PoptOpts * pInOptionsTable);
			virtual									~Popt();
			virtual inline int						parseResult(void) const		{return mResult;}
			virtual inline bool						isGood (void) const			{return parseResult() == -1;}
			virtual inline							operator bool() const		{return isGood();}
			virtual inline const std::string &		errorStr (void) const		{return mError;}
			virtual inline const NTV2StringList &	otherArgs (void) const		{return mOtherArgs;}
		private:
			poptContext		mContext;
			int				mResult;
			std::string		mError;
			NTV2StringList	mOtherArgs;
	};

	static bool	BFT(void);

};	//	CNTV2DemoCommon


//	These AJA_NTV2_AUDIO_RECORD* macros can, if enabled, record audio samples into a file in the current directory.
//	Optionally used in the CNTV2Capture demo.
#if defined(AJA_RAW_AUDIO_RECORD)
	#include "ntv2debug.h"					//	For NTV2DeviceString
	#include <fstream>						//	For ofstream
	//	To open the raw audio file in Audacity -- see http://audacity.sourceforge.net/ ...
	//		1)	Choose File => Import => Raw Data...
	//		2)	Select "Signed 32 bit PCM", Little/No/Default Endian, "16 Channels" (or 8 if applicable), "48000" sample rate.
	//		3)	Click "Import"
	#define		AJA_NTV2_AUDIO_RECORD_BEGIN		ostringstream	_filename;														\
												_filename	<< ::NTV2DeviceString(mDeviceID) << "-" << mDevice.GetIndexNumber()	\
															<< "." << ::NTV2ChannelToString(mConfig.fInputChannel,true)					\
															<< "." << ::NTV2InputSourceToString(mConfig.fInputSource, true)				\
															<< "." << ::NTV2VideoFormatToString(mVideoFormat)					\
															<< "." << ::NTV2AudioSystemToString(mAudioSystem, true)				\
															<< "." << AJAProcess::GetPid()										\
															<< ".raw";															\
												ofstream _ostrm(_filename.str(), ios::binary);

	#define		AJA_NTV2_AUDIO_RECORD_DO		if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))									\
													if (pFrameData->fAudioBuffer)												\
														_ostrm.write(pFrameData->AudioBuffer(),									\
																	streamsize(pFrameData->NumCapturedAudioBytes()));

	#define		AJA_NTV2_AUDIO_RECORD_END		
#elif defined(AJA_WAV_AUDIO_RECORD)
	#include "ntv2debug.h"					//	For NTV2DeviceString
	#include "ajabase/common/wavewriter.h"	//	For AJAWavWriter
	#define		AJA_NTV2_AUDIO_RECORD_BEGIN		ostringstream	_wavfilename;														\
												_wavfilename	<< ::NTV2DeviceString(mDeviceID) << "-" << mDevice.GetIndexNumber()	\
																<< "." << ::NTV2ChannelToString(mConfig.fInputChannel,true)			\
																<< "." << ::NTV2InputSourceToString(mConfig.fInputSource, true)		\
																<< "." << ::NTV2VideoFormatToString(mVideoFormat)					\
																<< "." << ::NTV2AudioSystemToString(mAudioSystem, true)				\
																<< "." << AJAProcess::GetPid()										\
																<< ".wav";															\
												const int		_wavMaxNumAudChls(mDevice.features().GetMaxAudioChannels());		\
												AJAWavWriter	_wavWriter (_wavfilename.str(),										\
																			AJAWavWriterAudioFormat(_wavMaxNumAudChls, 48000, 32));	\
												_wavWriter.open();

	#define		AJA_NTV2_AUDIO_RECORD_DO		if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))										\
													if (pFrameData->fAudioBuffer)													\
														if (_wavWriter.IsOpen())													\
															_wavWriter.write(pFrameData->AudioBuffer(), pFrameData->NumCapturedAudioBytes());

	#define		AJA_NTV2_AUDIO_RECORD_END		if (_wavWriter.IsOpen())															\
													_wavWriter.close();
#else
	#define		AJA_NTV2_AUDIO_RECORD_BEGIN		
	#define		AJA_NTV2_AUDIO_RECORD_DO			
	#define		AJA_NTV2_AUDIO_RECORD_END		
#endif

//	Optionally used in the CNTV2Capture4K demo.
#if defined(AJA_RECORD_MLAUDIO)
	#include <fstream>
	#define	AJA_NTV2_MLAUDIO_RECORD_BEGIN		ofstream ofs1, ofs2;												\
												if (mConfig.fNumAudioLinks > 1)										\
												{																	\
													ofs1.open("temp1.raw", ios::out | ios::trunc | ios::binary);	\
													ofs2.open("temp2.raw", ios::out | ios::trunc | ios::binary);	\
												}

	#define	AJA_NTV2_MLAUDIO_RECORD_DO			if (mConfig.fNumAudioLinks > 1)																\
												{	const ULWord halfBytes (pFrameData->NumCapturedAudioBytes() / 2);						\
													ofs1.write(pFrameData->AudioBuffer(), halfBytes);										\
													NTV2Buffer lastHalf (pFrameData->fAudioBuffer.GetHostAddress(halfBytes), halfBytes);	\
													ofs2.write(lastHalf, lastHalf.GetByteCount());											\
												}

	#define	AJA_NTV2_MLAUDIO_RECORD_END			if (mConfig.fNumAudioLinks > 1)				\
												{											\
													ofs1.close();							\
													ofs2.close();							\
												}
#else
	#define	AJA_NTV2_MLAUDIO_RECORD_BEGIN		
	#define	AJA_NTV2_MLAUDIO_RECORD_DO			
	#define	AJA_NTV2_MLAUDIO_RECORD_END			
#endif

#endif	//	_NTV2DEMOCOMMON_H
