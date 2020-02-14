/**
	@file		ntv2vpid.h
	@brief		Declares the CNTV2VPID class. See SMPTE 352 standard for details.
	@copyright	(C) 2012-2020 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2VPID_H
#define NTV2VPID_H

#include "ajaexport.h"
#include "ntv2publicinterface.h"
#include "ajabase/system/info.h"

#if defined(AJALinux)
#include <stdio.h>
#endif

/**
    @brief	A convenience class that simplifies encoding or decoding the 4-byte VPID payload
			that can be read or written from/to VPID registers.
**/
class AJAExport CNTV2VPID
{
public:
	/**
		@name	Construction, Destruction, Copying, Assigning
	**/
	///@{
								CNTV2VPID (const ULWord inData = 0);
								CNTV2VPID (const CNTV2VPID & other);
	virtual CNTV2VPID &			operator = (const CNTV2VPID & inRHS);
	virtual inline				~CNTV2VPID ()							{}
	///@}

	/**
		@name	Inquiry
	**/
	///@{
	virtual inline ULWord			GetVPID (void) const					{return m_uVPID;}	///< @return	My current 4-byte VPID value.
	virtual VPIDVersion				GetVersion (void) const;
	virtual NTV2VideoFormat			GetVideoFormat (void) const;
	virtual bool					IsStandard3Ga (void) const;
	virtual bool					IsStandardMultiLink4320 (void) const;
	virtual bool					IsStandardTwoSampleInterleave (void) const;
	virtual VPIDStandard			GetStandard (void) const;
	virtual bool					GetProgressiveTransport (void) const;
	virtual bool					GetProgressivePicture (void) const;
	virtual VPIDPictureRate			GetPictureRate (void) const;
	virtual bool					GetImageAspect16x9 (void) const;
	virtual VPIDSampling			GetSampling (void) const;
	virtual VPIDChannel				GetChannel (void) const;
	virtual VPIDChannel				GetDualLinkChannel (void) const;
	virtual VPIDBitDepth			GetBitDepth (void) const;
	virtual inline bool				IsValid (void) const			{return GetVersion() == VPIDVersion_1;}	///< @return	True if valid;  otherwise false.
	virtual AJALabelValuePairs &	GetInfo (AJALabelValuePairs & outInfo) const;
	virtual NTV2VPIDXferChars		GetTransferCharacteristics (void) const;
	virtual NTV2VPIDColorimetry		GetColorimetry (void) const;
	virtual NTV2VPIDLuminance		GetLuminance (void) const;				
	virtual std::ostream &			Print (std::ostream & ostrm) const;
	///@}

	/**
		@name	Changing
	**/
	///@{
	virtual inline void			SetVPID (const ULWord inData)			{m_uVPID = inData;}

	virtual bool				SetVPID (const NTV2VideoFormat		inVideoFormat,
										const NTV2FrameBufferFormat	inFrameBufferFormat,
										const bool					inIsProgressive,
										const bool					inIs16x9Aspect,
										const VPIDChannel			inVPIDChannel);

	virtual bool				SetVPID (const NTV2VideoFormat	inOutputFormat,
										const bool				inIsDualLink,
										const bool				inIs48BitRGBFormat,
										const bool				inIsOutput3Gb,
										const bool				inIsSMPTE425,
										const VPIDChannel		inVPIDhannel);


	
	virtual void				SetVersion (const VPIDVersion inVersion);
	virtual void				SetStandard (const VPIDStandard inStandard);
	virtual void				SetProgressiveTransport (const bool inIsProgressiveTransport);
	virtual void				SetProgressivePicture (const bool inIsProgressivePicture);
	virtual void				SetPictureRate (const VPIDPictureRate inPictureRate);
	virtual void				SetImageAspect16x9 (const bool inIs16x9Aspect);
	virtual void				SetSampling (const VPIDSampling inSampling);
	virtual void				SetChannel (const VPIDChannel inChannel);
	virtual void				SetDualLinkChannel (const VPIDChannel inChannel);
	virtual void				SetBitDepth (const VPIDBitDepth inBitDepth);
	virtual void				SetTransferCharacteristics (const NTV2VPIDXferChars inXferChars);
	virtual void				SetColorimetry (const NTV2VPIDColorimetry inColorimetry);
	virtual void				SetLuminance (const NTV2VPIDLuminance inLuminance);
								
	///@}


	/**
		@name	Class Methods
	**/
	///@{
	static bool					SetVPIDData (ULWord &					outVPID,
											const NTV2VideoFormat		inOutputFormat,
											const NTV2FrameBufferFormat	inFrameBufferFormat,
											const bool					inIsProgressive,
											const bool					inIs16x9Aspect,
											const VPIDChannel			inVPIDChannel,
											const bool					inUseVPIDChannel = true);		// defaults to using VPID channel

	static bool					SetVPIDData (ULWord &				outVPID,
											const NTV2VideoFormat	inOutputFormat,
											const bool				inIsDualLinkRGB,
											const bool				inIsRGB48Bit,
											const bool				inIsOutput3Gb,
											const bool				inIsSMPTE425,
											const VPIDChannel		inVPIDChannel,
											const bool				inUseVPIDChannel = true,
											const bool				inOutputIs6G = false,
											const bool				inOutputIs12G = false,
											const NTV2VPIDXferChars	inXferChars = NTV2_VPID_TC_SDR_TV,
											const NTV2VPIDColorimetry	inColorimetry = NTV2_VPID_Color_Rec709,
											const NTV2VPIDLuminance	inLuminance = NTV2_VPID_Luminance_YCbCr);

	static const std::string VersionString(VPIDVersion version);
	static const std::string StandardString (VPIDStandard std);
	static const std::string PictureRateString (VPIDPictureRate rate);
	static const std::string SamplingString (VPIDSampling sample);
	static const std::string ChannelString (VPIDChannel chan);
	static const std::string DynamicRangeString (VPIDDynamicRange range);
	static const std::string BitDepthString(VPIDBitDepth depth);
	static const std::string LinkString(VPIDLink link);
	static const std::string AudioString(VPIDAudio audio);
	#if !defined (NTV2_DEPRECATE)
		virtual VPIDDynamicRange NTV2_DEPRECATED_f(GetDynamicRange (void) const);
		virtual void NTV2_DEPRECATED_f(SetDynamicRange (const VPIDDynamicRange inDynamicRange));	
		static inline NTV2_DEPRECATED_f(bool	SetVPIDData (ULWord *				pOutVPID,
														const NTV2VideoFormat		inOutputFormat,
														const NTV2FrameBufferFormat	inFrameBufferFormat,
														const bool					inIsProgressive,
														const bool					inIs16x9Aspect,
														const VPIDChannel			inVPIDChannel,
														const bool					inUseVPIDChannel = true))
											{return pOutVPID ? SetVPIDData (*pOutVPID, inOutputFormat, inFrameBufferFormat, inIsProgressive, inIs16x9Aspect, inVPIDChannel, inUseVPIDChannel) : false;}

		static inline NTV2_DEPRECATED_f(bool	SetVPIDData (ULWord *				pOutVPID,
														const NTV2VideoFormat	inOutputFormat,
														const bool				inDualLinkRGB,
														const bool				inIsRGB48Bit,
														const bool				inIsOutput3Gb,
														const bool				inIsSMPTE425,
														const VPIDChannel		inVPIDChannel,
														const bool				inUseChannel = true))
											{return pOutVPID ? SetVPIDData (*pOutVPID, inOutputFormat, inDualLinkRGB, inIsRGB48Bit, inIsOutput3Gb, inIsSMPTE425, inVPIDChannel, inUseChannel) : false;}

		virtual inline NTV2_DEPRECATED_f(void	Init (void))				{}		///< @deprecated	Obsolete. Do not use.
	#endif	//	!defined (NTV2_DEPRECATE)
	///@}

private:
	ULWord	m_uVPID;	///< @brief	My 32-bit VPID data value

};	//	CNTV2VPID

AJAExport std::ostream &	operator << (std::ostream & ostrm, const CNTV2VPID & inData);

#endif	//	NTV2VPID_H
