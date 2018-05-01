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
	class AJAExport NTV2_DEPRECATED_CLASS CNTV2Status : public CNTV2Card
	{
		public:
			inline NTV2_DEPRECATED_f(			CNTV2Status ())				{}

			NTV2_DEPRECATED_f(					CNTV2Status (UWord			inDeviceIndex,
															bool			displayErrorMessage	= false,
															UWord			ulBoardType			= BOARDTYPE_NTV2,
															const char *	pInHostname			= NULL));
			virtual								~CNTV2Status ();

		public:
			AJA_VIRTUAL NTV2_DEPRECATED_f(void	GetBoardString						(std::string & outString));
			AJA_VIRTUAL NTV2_DEPRECATED_f(void	GetFrameBufferVideoFormatString		(std::string & outString));

			AJA_VIRTUAL NTV2_DEPRECATED_f(void	GetInput1VideoFormatString			(std::string & outString));
			AJA_VIRTUAL NTV2_DEPRECATED_f(void	GetInput2VideoFormatString			(std::string & outString));
			AJA_VIRTUAL NTV2_DEPRECATED_f(void	GetInput3VideoFormatString			(std::string & outString));
			AJA_VIRTUAL NTV2_DEPRECATED_f(void	GetInput4VideoFormatString			(std::string & outString));
			AJA_VIRTUAL NTV2_DEPRECATED_f(void	GetInput5VideoFormatString			(std::string & outString));
			AJA_VIRTUAL NTV2_DEPRECATED_f(void	GetInput6VideoFormatString			(std::string & outString));
			AJA_VIRTUAL NTV2_DEPRECATED_f(void	GetInput7VideoFormatString			(std::string & outString));
			AJA_VIRTUAL NTV2_DEPRECATED_f(void	GetInput8VideoFormatString			(std::string & outString));

			AJA_VIRTUAL NTV2_DEPRECATED_f(void	GetHDMIInputVideoFormatString		(std::string & outString));
			AJA_VIRTUAL NTV2_DEPRECATED_f(void	GetAnalogInputVideoFormatString		(std::string & outString));
			AJA_VIRTUAL NTV2_DEPRECATED_f(void	GetReferenceVideoFormatString		(std::string & outString));

			AJA_VIRTUAL NTV2_DEPRECATED_f(void	GetInputVideoFormatString			(int inputNum, std::string & outString));

			static NTV2_DEPRECATED_f(void			GetVideoFormatString				(NTV2VideoFormat format, std::string & outString));
			static NTV2_DEPRECATED_f(void			GetVideoStandardString				(NTV2Standard standard, std::string & outString));
			static NTV2_DEPRECATED_f(void			GetFrameRateString					(NTV2FrameRate frameRate, std::string & outString));

	};	//	CNTV2Status

#endif	//	!defined (NTV2_DEPRECATE)

#endif	//	NTV2STATUS_H
