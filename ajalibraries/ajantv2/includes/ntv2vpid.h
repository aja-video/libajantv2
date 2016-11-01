/**
	@file		ntv2vpid.h
	@brief		Declares the CNTV2VPID class. See SMPTE 352 standard for details.
	@copyright	(C) 2012-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2VPID_H
#define NTV2VPID_H

#include "ajaexport.h"
#include "ntv2publicinterface.h"

#if defined(AJALinux)
#include <stdio.h>
//#include "ntv2winlinhacks.h"
#endif

class AJAExport CNTV2VPID
{
public:
	//	Construction & Destruction
								CNTV2VPID ();
								CNTV2VPID (const CNTV2VPID & other);
	virtual CNTV2VPID &			operator = (const CNTV2VPID & inRHS);
	virtual						~CNTV2VPID ()							{}

	virtual inline void			SetVPID (const ULWord inData)			{m_uVPID = inData;}
	virtual inline ULWord		GetVPID (void) const					{return m_uVPID;}

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

	virtual bool				IsStandard3Ga (void) const;

	virtual bool				IsStandardTwoSampleInterleave (void) const;

	virtual NTV2VideoFormat		GetVideoFormat (void) const;
	
	virtual void				SetVersion (const VPIDVersion inVersion);
	virtual VPIDVersion			GetVersion (void) const;

	virtual void				SetStandard (const VPIDStandard inStandard);
	virtual VPIDStandard		GetStandard (void) const;

	virtual void				SetProgressiveTransport (const bool inIsProgressiveTransport);
	virtual bool				GetProgressiveTransport (void) const;

	virtual void				SetProgressivePicture (const bool inIsProgressivePicture);
	virtual bool				GetProgressivePicture (void) const;

	virtual void				SetPictureRate (const VPIDPictureRate inPictureRate);
	virtual VPIDPictureRate		GetPictureRate (void) const;

	virtual void				SetImageAspect16x9 (const bool inIs16x9Aspect);
	virtual bool				GetImageAspect16x9 (void) const;

	virtual void				SetSampling (const VPIDSampling inSampling);
	virtual VPIDSampling		GetSampling (void) const;

	virtual void				SetChannel (const VPIDChannel inChannel);
	virtual VPIDChannel			GetChannel (void) const;
	
	virtual void				SetDualLinkChannel (const VPIDChannel inChannel);
	virtual VPIDChannel			GetDualLinkChannel (void) const;

	virtual void				SetDynamicRange (const VPIDDynamicRange inDynamicRange);
	virtual VPIDDynamicRange	GetDynamicRange (void) const;

	virtual void				SetBitDepth (const VPIDBitDepth inBitDepth);
	virtual VPIDBitDepth		GetBitDepth (void) const;

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
											const bool				inUseVPIDChannel = true);		// defaults to using VPID channel
	#if !defined (NTV2_DEPRECATE)
		static NTV2_DEPRECATED inline bool	SetVPIDData (ULWord *					pOutVPID,
														const NTV2VideoFormat		inOutputFormat,
														const NTV2FrameBufferFormat	inFrameBufferFormat,
														const bool					inIsProgressive,
														const bool					inIs16x9Aspect,
														const VPIDChannel			inVPIDChannel,
														const bool					inUseVPIDChannel = true)
											{return pOutVPID ? SetVPIDData (*pOutVPID, inOutputFormat, inFrameBufferFormat, inIsProgressive, inIs16x9Aspect, inVPIDChannel, inUseVPIDChannel) : false;}

		static NTV2_DEPRECATED inline bool	SetVPIDData (ULWord *				pOutVPID,
														const NTV2VideoFormat	inOutputFormat,
														const bool				inDualLinkRGB,
														const bool				inIsRGB48Bit,
														const bool				inIsOutput3Gb,
														const bool				inIsSMPTE425,
														const VPIDChannel		inVPIDChannel,
														const bool				inUseChannel = true)
											{return pOutVPID ? SetVPIDData (*pOutVPID, inOutputFormat, inDualLinkRGB, inIsRGB48Bit, inIsOutput3Gb, inIsSMPTE425, inVPIDChannel, inUseChannel) : false;}

		virtual NTV2_DEPRECATED	inline void	Init (void)				{}		///< @deprecated	Obsolete. Do not use.
	#endif	//	!defined (NTV2_DEPRECATE)

private:
	ULWord	m_uVPID;	///< @brief	My 32-bit VPID data value

};	//	CNTV2VPID

#endif	//	NTV2VPID_H
