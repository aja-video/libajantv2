/**
	@file		xena2vidproc.h
	@deprecated	Use the CNTV2Card class instead.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef XENA2VIDPROC_H
#define XENA2VIDPROC_H

#include "ajaexport.h"
#include "ntv2testpattern.h"

#if defined (NTV2_DEPRECATE)

	typedef	CNTV2Card	CXena2VidProc;	///< @deprecated	Use CNTV2Card instead.

#else

	class AJAExport NTV2_DEPRECATED CXena2VidProc : public CNTV2TestPattern
	{
		public:
			explicit inline								CXena2VidProc ()		 {};
														CXena2VidProc (UWord inDeviceIndex, bool displayErrorMessage = false, UWord ulBoardType = DEVICETYPE_NTV2);
			virtual										~CXena2VidProc ();

		public:
			// This tells the video processing whether to ignore key
			// use key and assumed it has already been shaped or not
			//	NTV2Channel here means:		Channel1 == foreground		Channel2 == background
			AJA_VIRTUAL NTV2_DEPRECATED bool			SetXena2VidProcInputControl (NTV2Channel channel, NTV2MixerKeyerInputControl inputControl);
			AJA_VIRTUAL NTV2_DEPRECATED bool			GetXena2VidProcInputControl (NTV2Channel channel, NTV2MixerKeyerInputControl * inputControl);
			AJA_VIRTUAL NTV2_DEPRECATED bool			SetXena2VidProc2InputControl (NTV2Channel channel, NTV2MixerKeyerInputControl inputControl);
			AJA_VIRTUAL NTV2_DEPRECATED bool			GetXena2VidProc2InputControl (NTV2Channel channel, NTV2MixerKeyerInputControl * inputControl);
			AJA_VIRTUAL NTV2_DEPRECATED bool			SetXena2VidProc3InputControl (NTV2Channel channel, NTV2MixerKeyerInputControl inputControl);
			AJA_VIRTUAL NTV2_DEPRECATED bool			GetXena2VidProc3InputControl (NTV2Channel channel, NTV2MixerKeyerInputControl * inputControl);
			AJA_VIRTUAL NTV2_DEPRECATED bool			SetXena2VidProc4InputControl (NTV2Channel channel, NTV2MixerKeyerInputControl inputControl);
			AJA_VIRTUAL NTV2_DEPRECATED bool			GetXena2VidProc4InputControl (NTV2Channel channel, NTV2MixerKeyerInputControl * inputControl);

			AJA_VIRTUAL NTV2_DEPRECATED bool			SetXena2VidProcMode (NTV2MixerKeyerMode mode);
			AJA_VIRTUAL NTV2_DEPRECATED bool			GetXena2VidProcMode (NTV2MixerKeyerMode * mode);
			AJA_VIRTUAL NTV2_DEPRECATED bool			SetXena2VidProc2Mode (NTV2MixerKeyerMode mode);
			AJA_VIRTUAL NTV2_DEPRECATED bool			GetXena2VidProc2Mode (NTV2MixerKeyerMode * mode);
			AJA_VIRTUAL NTV2_DEPRECATED bool			SetXena2VidProc3Mode (NTV2MixerKeyerMode mode);
			AJA_VIRTUAL NTV2_DEPRECATED bool			GetXena2VidProc3Mode (NTV2MixerKeyerMode * mode);
			AJA_VIRTUAL NTV2_DEPRECATED bool			SetXena2VidProc4Mode (NTV2MixerKeyerMode mode);
			AJA_VIRTUAL NTV2_DEPRECATED bool			GetXena2VidProc4Mode (NTV2MixerKeyerMode * mode);

			AJA_VIRTUAL NTV2_DEPRECATED void			SetSplitMode (NTV2SplitMode splitMode);
			AJA_VIRTUAL NTV2_DEPRECATED NTV2SplitMode	GetSplitMode (void);
			AJA_VIRTUAL NTV2_DEPRECATED void			SetSplitParameters (Fixed_ position, Fixed_ softness);
			AJA_VIRTUAL NTV2_DEPRECATED void			SetSlitParameters (Fixed_ start, Fixed_ width);

			AJA_VIRTUAL NTV2_DEPRECATED void			SetMixCoefficient (Fixed_ coefficient);
			AJA_VIRTUAL NTV2_DEPRECATED Fixed_			GetMixCoefficient (void);
			AJA_VIRTUAL NTV2_DEPRECATED void			SetMix2Coefficient (Fixed_ coefficient);
			AJA_VIRTUAL NTV2_DEPRECATED Fixed_			GetMix2Coefficient (void);
			AJA_VIRTUAL NTV2_DEPRECATED void			SetMix3Coefficient (Fixed_ coefficient);
			AJA_VIRTUAL NTV2_DEPRECATED Fixed_			GetMix3Coefficient (void);
			AJA_VIRTUAL NTV2_DEPRECATED void			SetMix4Coefficient (Fixed_ coefficient);
			AJA_VIRTUAL NTV2_DEPRECATED Fixed_			GetMix4Coefficient (void);

	};	//	CXena2VidProc

#endif	//	!defined (NTV2_DEPRECATE)

#endif	//	XENA2VIDPROC_H
