/**
	@file		ntv2captiontranslator708to708.h
	@brief		Declares the CNTV2CaptionTranslator708to708 class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CEA708to708_TRANSLATOR_
#define __NTV2_CEA708to708_TRANSLATOR_

#include "ntv2captiondecoder708.h"
#include "ntv2captionencoder708.h"


#ifdef MSWindows
	#include "windows.h"
#endif


/**
	@brief	I translate/transform CEA-708 ("DTVCC") captions from one frame rate to another.
**/
class CNTV2CaptionTranslator708to708;
typedef AJARefPtr <CNTV2CaptionTranslator708to708>	CNTV2CaptionTranslator708to708Ptr;


class AJAExport CNTV2CaptionTranslator708to708 : public CNTV2CaptionLogConfig
{
	//	Class Methods
	public:
		/**
			@brief		Creates a new CNTV2CaptionEncoder708 instance.
			@param[out]	outEncoder	Receives the newly-created encoder instance.
			@return		True if successful; otherwise False.
		**/
		static bool		Create (CNTV2CaptionTranslator708to708Ptr & outEncoder);


	//	Instance Methods
	public:
		virtual			~CNTV2CaptionTranslator708to708 ();

		virtual void	Reset (void);

		virtual bool	GrabInputSmpte334AndParse (const UByte *				pVideo,
													const NTV2VideoFormat		videoFormat,
													const NTV2FrameBufferFormat	pixelFormat,
													bool &						outHasParityErrors);

		virtual bool	CreateSMPTE334Anc (const NTV2FrameRate inOutputFrameRate, const NTV2Line21Field inField, UWordPtr & outAncPacketData, size_t & outSize);
		virtual bool	CreateSMPTE334Anc (const NTV2FrameRate inOutputFrameRate, const NTV2Line21Field inField);

		virtual bool	OutputSMPTE334Anc (void * pFrameBuffer, const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inPixelFormat, const ULWord inLineNumber = 0);

		virtual bool	CopyDecoderDataToEncoder (NTV2FrameRate outputFrameRate, NTV2Line21Field field);

		virtual void	Set608TestIDMode (bool bTest);

		virtual NTV2CaptionLogMask	SetLogMask (const NTV2CaptionLogMask inLogMask);


	//	Private Instance Methods
	private:
		virtual bool	Combine708CaptionServiceData (NTV2FrameRate frameRate);
		virtual unsigned	MaxCaptionChannelDataForFrameRate (NTV2FrameRate ntv2Rate);
		virtual bool	AddServiceServiceBlockData (const size_t svcIndex, UByte * pEncodeData, size_t index, const size_t maxIndex, size_t & outEndIndex);

		explicit									CNTV2CaptionTranslator708to708 ();
		explicit									CNTV2CaptionTranslator708to708 (const CNTV2CaptionTranslator708to708 & inTranslatorToCopy);
		virtual CNTV2CaptionTranslator708to708 &	operator = (const CNTV2CaptionTranslator708to708 & inTranslatorToCopy);


	//	INSTANCE DATA
	private:
		CNTV2CaptionDecoder708Ptr	m708Decoder;		///< @brief	CEA-708 decoder class to receive incoming captions
		CNTV2CaptionEncoder708Ptr	m708Encoder;		///< @brief	CEA-708 encoder class used to create 708 output
		//int						mDebugPrintOffset;	///< @brief	Offset added to debug levels (used in cases where there are multiple instances,
};

#endif	// __NTV2_CEA708to708_TRANSLATOR_
