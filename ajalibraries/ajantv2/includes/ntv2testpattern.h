/**
	@file		ntv2testpattern.h
	@brief		Declares the CNTV2TestPattern class.
	@copyright	(C) 2004-2018 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2TESTPATTERN_H
#define NTV2TESTPATTERN_H

#include "ajaexport.h"
#include "ntv2status.h"
#include "string.h"
#include "ntv2utils.h"

/**	MOVED INTO cntv2card.h
typedef struct
{
	int				startLine;
	int				endLine;
	const ULWord *	data;
} SegmentDescriptor;

const UWord NumTestPatternSegments = 8;

typedef struct
{
	const char *		name;
	SegmentDescriptor	segmentDescriptor [NTV2_NUM_STANDARDS] [NumTestPatternSegments];
} SegmentTestPatternData;
**/

#if defined (NTV2_DEPRECATE)
	typedef	CNTV2Card	CNTV2TestPattern;	///< @deprecated	Use CNTV2Card instead.
#else
/**
	@deprecated	Use CNTV2Card instead.
**/
class AJAExport NTV2_DEPRECATED_CLASS CNTV2TestPattern : public CNTV2Status
{
public:
	NTV2_DEPRECATED_f(					CNTV2TestPattern ());
	NTV2_DEPRECATED_f(					CNTV2TestPattern (UWord			inDeviceIndex,
															bool		displayErrorMessage	= false,
															UWord		dwCardTypes			= DEVICETYPE_NTV2,
															bool		autoRouteOnXena2	= false,
															const char	hostname[]			= 0,	// Non-null: card on remote host
															NTV2Channel	channel				= NTV2_CHANNEL1));
	virtual								~CNTV2TestPattern ();

public:
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(void						SetChannel (NTV2Channel channel))									{_channel = channel;}
    AJA_VIRTUAL inline NTV2_DEPRECATED_f(NTV2Channel				GetChannel (void))													{return _channel;}
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(void						SetSignalMask (UWord signalMask))									{_signalMask = signalMask & 0x7;}
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(UWord						GetSignalMask (void) const)											{return _signalMask;}
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(void						SetTestPatternFrameBufferFormat (NTV2FrameBufferFormat fbFormat))	{_fbFormat = fbFormat;}
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(NTV2FrameBufferFormat		GetTestPatternFrameBufferFormat (void) const)						{return _fbFormat;}
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(void						SetDualLinkTestPatternOutputEnable (bool enable))					{_dualLinkOutputEnable = enable;}
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool						GetDualLinkTestPatternOutputEnable (void) const)						{return _dualLinkOutputEnable;}
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(const TestPatternList &	GetTestPatternList (void) const)										{return _testPatternList;}

	AJA_VIRTUAL inline NTV2_DEPRECATED_f(void						SetClientDownloadBuffer (ULWord * buffer))								{_clientDownloadBuffer = buffer;}
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(ULWord *					GetClientDownloadBuffer (void) const)									{return _clientDownloadBuffer;}
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(void						SetTestPatternDMAEnable (bool enable))									{_bDMAtoBoard = enable;}
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool						GetTestPatternDMAEnable (void) const)									{return _bDMAtoBoard; }
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(void						EnableFlipFlipPage (bool enable))										{_flipFlopPage = enable;}
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool						GetEnableFlipFlipPage (void) const)										{return _flipFlopPage;}
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(void						SetAutoRouteOnXena2 (bool autoRoute))									{_autoRouteOnXena2 = autoRoute;}
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool						GetAutoRouteOnXena2 (void) const)										{return _autoRouteOnXena2;}
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool					DownloadTestPattern (UWord testPatternNumber));
	AJA_VIRTUAL NTV2_DEPRECATED_f(void					DownloadTestPattern (char * testPatternName));
	AJA_VIRTUAL NTV2_DEPRECATED_f(void					DownloadBlackTestPattern (void));
	AJA_VIRTUAL NTV2_DEPRECATED_f(void					DownloadBorderTestPattern (void));
	AJA_VIRTUAL NTV2_DEPRECATED_f(void					DownloadSlantRampTestPattern (void));
	AJA_VIRTUAL NTV2_DEPRECATED_f(void					DownloadYCbCrSlantRampTestPattern (void));
	AJA_VIRTUAL NTV2_DEPRECATED_f(void					Download48BitRGBSlantRampTestPattern (void));
	AJA_VIRTUAL NTV2_DEPRECATED_f(void					DownloadVerticalSweepTestPattern (void));
	AJA_VIRTUAL NTV2_DEPRECATED_f(void					DownloadZonePlateTestPattern (void));

	AJA_VIRTUAL NTV2_DEPRECATED_f(void					RenderTestPatternToBuffer (UWord testPatternNumber, ULWord * buffer));
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool					RenderTestPatternBuffer (NTV2Channel channel, UByte * buffer, NTV2VideoFormat videoFormat, NTV2FrameBufferFormat fbFormat, ULWord width, ULWord height, ULWord rowBytes));
	AJA_VIRTUAL NTV2_DEPRECATED_f(void					DownloadTestPatternBuffer (ULWord * buffer, ULWord size = 0));
	AJA_VIRTUAL NTV2_DEPRECATED_f(ULWord					GetPatternBufferSize (ULWord * width = 0, ULWord * height = 0, ULWord * rowBytes = 0, ULWord * firstLine = 0));
	
	AJA_VIRTUAL NTV2_DEPRECATED_f(int						MakeSineWaveVideo (double radians, bool bChroma));
	AJA_VIRTUAL NTV2_DEPRECATED_f(void					ConvertLinePixelFormat (UWord * unPackedBuffer, ULWord * packedBuffer, int numPixels));

#ifdef AJAMac
	AJA_VIRTUAL NTV2_DEPRECATED_f(void					DownloadRGBPicture (char * pSrc, ULWord srcWidthPixels, ULWord srcHeightPixels, ULWord srcRowBytes));
#endif
#if !defined(NTV2_DEPRECATE_13_0)
	//LocalLoadTestPattern allows the generator to build the pattern independent of global controls to generate independent formats when inconverter is on.
	AJA_VIRTUAL NTV2_DEPRECATED_f(void					LocalLoadBarsTestPattern (UWord testPatternNumber, NTV2Standard standard));
#endif	//	!defined(NTV2_DEPRECATE_13_0)

protected:   // Methods
	void												InitNTV2TestPattern (void);
	AJA_VIRTUAL void									DownloadSegmentedTestPattern (SegmentTestPatternData * pTestPatternSegmentData);
	AJA_VIRTUAL void									AdjustFor2048x1080 (ULWord & numPixels, ULWord & linePitch);
	AJA_VIRTUAL void									AdjustFor4096x2160 (ULWord & numPixels, ULWord & linePitch, ULWord & numLines);
	AJA_VIRTUAL void									AdjustFor3840x2160 (ULWord & numPixels, ULWord & linePitch, ULWord & numLines);

protected:   // Data
	NTV2Channel				_channel;
	TestPatternList			_testPatternList;
	UWord					_signalMask;
	NTV2FrameBufferFormat	_fbFormat;
	bool					_dualLinkOutputEnable;
	bool					_bDMAtoBoard;
	bool					_autoRouteOnXena2;
	bool					_flipFlopPage;
	ULWord *				_clientDownloadBuffer;

};	//	CNTV2TestPattern

#endif	//	!defined (NTV2_DEPRECATE)

#endif	//	NTV2TESTPATTERN_H
