/**
	@file		ntv2vidproc.h
	@brief		Declares the CNTV2VidProc class.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2VIDPROC_H
#define NTV2VIDPROC_H

#include "ajaexport.h"
#include "ntv2testpattern.h"


#if defined (NTV2_DEPRECATE)
	typedef CNTV2Card	CNTV2VidProc;	///< @deprecated	Use CNTV2Card instead
#else
	/**
		@deprecated	Use CNTV2Card instead.
	**/
	class AJAExport NTV2_DEPRECATED CNTV2VidProc : public CNTV2TestPattern
	{
		//	Construction & Destruction
		public:
			NTV2_DEPRECATED explicit inline					CNTV2VidProc () {};
			NTV2_DEPRECATED									CNTV2VidProc (UWord inDeviceIndex, bool displayErrorMessage = false, UWord ulBoardType = DEVICETYPE_NTV2);
			NTV2_DEPRECATED									CNTV2VidProc (UWord inDeviceIndex, bool displayErrorMessage, UWord dwCardTypes = DEVICETYPE_NTV2, bool autoRouteOnXena2 = false, const char hostname[] = 0);
			virtual											~CNTV2VidProc ();

		public:
			AJA_VIRTUAL NTV2_DEPRECATED void				SetupDefaultVidProc (void);
			AJA_VIRTUAL NTV2_DEPRECATED void				DisableVidProc (void);

			AJA_VIRTUAL NTV2_DEPRECATED void				SetCh1VidProcMode (NTV2Ch1VidProcMode vidProcMode);
			AJA_VIRTUAL NTV2_DEPRECATED NTV2Ch1VidProcMode	GetCh1VidProcMode (void);
			AJA_VIRTUAL NTV2_DEPRECATED void				SetCh2OutputMode (NTV2Ch2OutputMode outputMode);
			AJA_VIRTUAL NTV2_DEPRECATED NTV2Ch2OutputMode	GetCh2OutputMode (void);

			AJA_VIRTUAL NTV2_DEPRECATED void				SetForegroundVideoCrosspoint (NTV2Crosspoint crosspoint);
			AJA_VIRTUAL NTV2_DEPRECATED void				SetForegroundKeyCrosspoint (NTV2Crosspoint crosspoint);
			AJA_VIRTUAL NTV2_DEPRECATED void				SetBackgroundVideoCrosspoint (NTV2Crosspoint crosspoint);
			AJA_VIRTUAL NTV2_DEPRECATED void				SetBackgroundKeyCrosspoint (NTV2Crosspoint crosspoint);
			AJA_VIRTUAL NTV2_DEPRECATED NTV2Crosspoint		GetForegroundVideoCrosspoint (void);
			AJA_VIRTUAL NTV2_DEPRECATED NTV2Crosspoint		GetForegroundKeyCrosspoint (void);
			AJA_VIRTUAL NTV2_DEPRECATED NTV2Crosspoint		GetBackgroundVideoCrosspoint (void);
			AJA_VIRTUAL NTV2_DEPRECATED NTV2Crosspoint		GetBackgroundKeyCrosspoint (void);

			AJA_VIRTUAL NTV2_DEPRECATED void				SetSplitMode (NTV2SplitMode splitMode);
			AJA_VIRTUAL NTV2_DEPRECATED NTV2SplitMode		GetSplitMode (void);
			AJA_VIRTUAL NTV2_DEPRECATED void				SetSplitParameters (Fixed_ position, Fixed_ softness);
			AJA_VIRTUAL NTV2_DEPRECATED void				SetSlitParameters (Fixed_ start, Fixed_ width);

			AJA_VIRTUAL NTV2_DEPRECATED void				SetMixCoefficient (Fixed_ coefficient);
			AJA_VIRTUAL NTV2_DEPRECATED Fixed_				GetMixCoefficient (void);

			AJA_VIRTUAL NTV2_DEPRECATED void				SetMatteColor (YCbCr10BitPixel ycbcrPixel);

			#ifdef MSWindows
				AJA_VIRTUAL NTV2_DEPRECATED void			SetMatteColor (COLORREF rgbColor);
			#endif   

			#ifdef AJALinux
				AJA_VIRTUAL NTV2_DEPRECATED void			SetMatteColor (AJARgb rgbColor);
			#endif

	};	//	CNTV2VidProc
#endif	//	defined (NTV2_DEPRECATE)

#endif	//	NTV2VIDPROC_H
