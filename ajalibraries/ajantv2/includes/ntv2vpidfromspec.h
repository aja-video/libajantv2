/**
	@file		ntv2vpidfromspec.h
	@brief		Declares functions for the straight C implementations of VPID generation from a specification.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
	@note		This file is included in driver builds. It must not contain any C++.
**/

#ifndef NTV2VPIDFROMSPEC_H
#define NTV2VPIDFROMSPEC_H

#include "ajaexport.h"
#include "ntv2publicinterface.h"

/**
	@brief	Contains all the information needed to generate a valid VPID
**/

typedef struct
{
	NTV2VideoFormat			videoFormat;			///< @brief	Specifies the format of the video stream.
	NTV2FrameBufferFormat	pixelFormat;			///< @brief Specifies the pixel format of the source of the video stream.
	bool					isRGBOnWire;			///< @brief	If true, the transport on the wire is RGB.
	bool					isOutputLevelA;			///< @brief	If true, the video stream will leave the device as a level A signal.
	bool					isOutputLevelB;			///< @brief	If true, the video stream will leave the device as a level B signal.
	bool					isDualLink;				///< @brief	If true, the video stream is part of a SMPTE 372 dual link signal.
	bool					isTwoSampleInterleave;	///< @brief	If true, the video stream is in SMPTE 425-3 two sample interleave format.
	bool					useChannel;				///< @brief	If true, the following vpidChannel value should be inserted into th VPID.
	VPIDChannel				vpidChannel;			///< @brief Specifies the channel number of the video stream.
	bool					isStereo;				///< @brief	If true, the video stream is part of a stereo pair.
	bool					isRightEye;				///< @brief	If true, the video stream is the right eye of a stereo pair.
	VPIDAudio				audioCarriage;			///< @brief	Specifies how audio is carried in additional channels.
} VPIDSpec;

/**
	@brief		Generates a VPID based on the supplied specification.
	@param[out]	pOutVPID	Specifies the location where the generated VPID will be stored.
	@param[in]	pInVPIDSpec	Specifies the location of the settings describing the VPID to be generated.
	@return		True if generation was successful, otherwise false.
**/

AJAExport	bool	SetVPIDFromSpec (ULWord * const			pOutVPID,
									 const VPIDSpec * const	pInVPIDSpec);

#endif	// NTV2VPIDFROMSPEC_H

