/**
	@file		ntv2status.h
	@deprecated	This module is obsolete. Use ntv2card.h instead.
	@copyright	(C) 2004-2014 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2STATUS_H
#define NTV2STATUS_H
#include <string>

#include "ajaexport.h"
#include "ntv2card.h"

#if defined (NTV2_DEPRECATE)

	typedef	CNTV2Card	CNTV2Status;	///< @deprecated	Use CNTV2Card instead.

#else
	/**
		@deprecated	Use CNTV2Card instead.
	**/
	class AJAExport NTV2_DEPRECATED CNTV2Status : public CNTV2Card
	{
		public:
			NTV2_DEPRECATED inline				CNTV2Status ()				{}

			NTV2_DEPRECATED						CNTV2Status (UWord			inDeviceIndex,
															bool			displayErrorMessage	= false,
															UWord			ulBoardType			= BOARDTYPE_NTV2,
															const char *	pInHostname			= NULL);
			virtual								~CNTV2Status ();

		public:
			AJA_VIRTUAL NTV2_DEPRECATED void	GetBoardString						(std::string & outString);
			AJA_VIRTUAL NTV2_DEPRECATED void	GetFrameBufferVideoFormatString		(std::string & outString);

			AJA_VIRTUAL NTV2_DEPRECATED void	GetInput1VideoFormatString			(std::string & outString);
			AJA_VIRTUAL NTV2_DEPRECATED void	GetInput2VideoFormatString			(std::string & outString);
			AJA_VIRTUAL NTV2_DEPRECATED void	GetInput3VideoFormatString			(std::string & outString);
			AJA_VIRTUAL NTV2_DEPRECATED void	GetInput4VideoFormatString			(std::string & outString);
			AJA_VIRTUAL NTV2_DEPRECATED void	GetInput5VideoFormatString			(std::string & outString);
			AJA_VIRTUAL NTV2_DEPRECATED void	GetInput6VideoFormatString			(std::string & outString);
			AJA_VIRTUAL NTV2_DEPRECATED void	GetInput7VideoFormatString			(std::string & outString);
			AJA_VIRTUAL NTV2_DEPRECATED void	GetInput8VideoFormatString			(std::string & outString);

			AJA_VIRTUAL NTV2_DEPRECATED void	GetHDMIInputVideoFormatString		(std::string & outString);
			AJA_VIRTUAL NTV2_DEPRECATED void	GetAnalogInputVideoFormatString		(std::string & outString);
			AJA_VIRTUAL NTV2_DEPRECATED void	GetReferenceVideoFormatString		(std::string & outString);

			AJA_VIRTUAL NTV2_DEPRECATED void	GetInputVideoFormatString			(int inputNum, std::string & outString);

			static NTV2_DEPRECATED void			GetVideoFormatString				(NTV2VideoFormat format, std::string & outString);
			static NTV2_DEPRECATED void			GetVideoStandardString				(NTV2Standard standard, std::string & outString);
			static NTV2_DEPRECATED void			GetFrameRateString					(NTV2FrameRate frameRate, std::string & outString);

	};	//	CNTV2Status

#endif	//	!defined (NTV2_DEPRECATE)

#endif	//	NTV2STATUS_H
