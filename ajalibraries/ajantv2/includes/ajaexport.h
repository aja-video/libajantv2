/**
	@file		ajaexport.h
	@copyright	Copyright (C) 2008-2016 AJA Video Systems, Inc.  Proprietary and Confidential information.
	@brief		Defines the import/export macros for producing DLLs or LIBs.
**/

#ifndef AJAEXPORT_H
#define AJAEXPORT_H

#ifdef MSWindows
	#ifndef AJASTATIC
		#ifdef AJADLL
			#pragma warning (disable : 4251)
			#ifdef AJADLL_BUILD
				#define AJAExport __declspec(dllexport)
			#else
				#define AJAExport __declspec(dllimport)
			#endif
		#else
			#define AJAExport
			#ifndef AJA_NO_AUTOIMPORT
			#endif
		#endif
	#else
		#define AJAExport
	#endif
#else
	#define AJAExport
#endif

#endif	//	AJAEXPORT_H
