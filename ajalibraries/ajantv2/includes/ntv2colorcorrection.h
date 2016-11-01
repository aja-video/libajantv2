/**
	@file		ntv2colorcorrection.h
	@brief		Declares the CNTV2ColorCorrection class.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/
#if 0
#ifndef NTV2ColorCorrection_H
#define NTV2ColorCorrection_H

#include "ajaexport.h"
#include "ntv2status.h"
#include <fstream>
#include <vector>

#if defined (NTV2_DEPRECATE)
	typedef	CNTV2Card	CNTV2ColorCorrection;	///< @deprecated	Use CNTV2Card instead.
#else
	/**
		@deprecated	Use CNTV2Card instead.
	**/
	class AJAExport NTV2_DEPRECATED CNTV2ColorCorrection : public CNTV2Status
	{
	public:  // Constructors
		explicit	CNTV2ColorCorrection ();
		explicit	CNTV2ColorCorrection (UWord boardNumber, bool displayErrorMessage = false, UWord ulBoardType = DEVICETYPE_NTV2);
		virtual		~CNTV2ColorCorrection();

	public:  // Methods
		AJA_VIRTUAL NTV2_DEPRECATED bool		SetupColorCorrectionPointers (bool ajamac = AJA_RETAIL_DEFAULT);
		AJA_VIRTUAL NTV2_DEPRECATED bool		SetBoard (UWord boardNumber);
		AJA_VIRTUAL NTV2_DEPRECATED inline void	SetChannel (NTV2Channel channel)		{_channel = channel;}
		AJA_VIRTUAL NTV2_DEPRECATED void		SetColorCorrectionEnable (bool enable);
		AJA_VIRTUAL NTV2_DEPRECATED void		PingPongColorCorrectionTable (void);
		AJA_VIRTUAL NTV2_DEPRECATED void		SetColorCorrectionValues (ColorCorrectionColor color,double gamma, double gain, double offset);
		AJA_VIRTUAL NTV2_DEPRECATED void		SetColorCorrectionGamma (ColorCorrectionColor colorChoice,double gamma);
		AJA_VIRTUAL NTV2_DEPRECATED void		SetColorCorrectionGain (ColorCorrectionColor colorChoice,double gain);
		AJA_VIRTUAL NTV2_DEPRECATED void		SetColorCorrectionOffset (ColorCorrectionColor colorChoice,double offset);

		
		AJA_VIRTUAL NTV2_DEPRECATED ULWord *	GetHWTableBaseAddress (ColorCorrectionColor colorChoice);
		AJA_VIRTUAL NTV2_DEPRECATED UWord *		GetTableBaseAddress (ColorCorrectionColor colorChoice);
		AJA_VIRTUAL NTV2_DEPRECATED void		BuildTables (void);
		AJA_VIRTUAL NTV2_DEPRECATED void		TransferTablesToHardware (void);

		// TransferTablesToBuffer
		// ccBuffer needs to be 512*4*3 bytes long.
		// This is suitable to pass to transferutocirculate
		AJA_VIRTUAL NTV2_DEPRECATED void		TransferTablesToBuffer (ULWord* ccBuffer);

		// Copy external LUTs (each double LUT[1024]) to/from internal buffers
		AJA_VIRTUAL NTV2_DEPRECATED void		SetTables(double *redLUT, double *greenLUT, double *blueLUT);
		AJA_VIRTUAL NTV2_DEPRECATED void		GetTables(double *redLUT, double *greenLUT, double *blueLUT);

		// Copy external LUTs (each double LUT[1024]) direct to/from hardware
		AJA_VIRTUAL NTV2_DEPRECATED void		SetTablesToHardware  (double *redLUT, double *greenLUT, double *blueLUT);
		AJA_VIRTUAL NTV2_DEPRECATED void		GetTablesFromHardware (double *redLUT, double *greenLUT, double *blueLUT);

	protected:  // Data
		void									InitNTV2ColorCorrection (void);
		void									FreeNTV2ColorCorrection (void);

		ULWord *		_pHWTableBaseAddress [CNTV2ColorCorrection::NUM_COLORS];	//	On-board base address
		UWord *			_pColorCorrectionTable [CNTV2ColorCorrection::NUM_COLORS];	//	Tables for holding calculations
		double			_Gamma [CNTV2ColorCorrection::NUM_COLORS];
		double			_Gain [CNTV2ColorCorrection::NUM_COLORS];
		double			_Offset [CNTV2ColorCorrection::NUM_COLORS];
		NTV2Channel		_channel;

	};	//	CNTV2ColorCorrection
#endif	//	!defined (NTV2_DEPRECATE)

#endif	//	NTV2ColorCorrection_H

#endif