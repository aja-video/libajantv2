/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2metale2e.cpp
	@brief		Header file for NTV2OutputTestPattern demonstration class
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2METALE2E_H
#define _NTV2METALE2E_H

#include "ntv2democommon.h"
#include "ntv2vpid.h"
/**
	@brief	I generate and transfer a test pattern into an AJA device's frame buffer for steady-state
			playout using NTV2TestPatternGen::DrawTestPattern and CNTV2Card::DMAWriteFrame.
**/
class NTV2MetalE2E
{
	//	Public Instance Methods
	public:

		/**
			@brief	Constructs me using the given configuration settings.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inConfig		Specifies the configuration parameters.
		**/
        NTV2MetalE2E (void);

        ~NTV2MetalE2E (void);

		/**
            @brief		Do something from the KonaX Xilinx Baremetal system
			@return		AJA_STATUS_SUCCESS if successful; otherwise another AJAStatus code if unsuccessful.
		**/
        AJAStatus		DoSomething (void);

        /**
            @brief	Sets up the genlock circuit
            @return		AJA_STATUS_SUCCESS if successful; otherwise another AJAStatus code if unsuccessful.
        **/
        AJAStatus       SetUpGenlock (void);

        /**
            @brief	Setup video formats and outputs
            @return		AJA_STATUS_SUCCESS if successful; otherwise another AJAStatus code if unsuccessful.
        **/
        AJAStatus       SetUpVideo (void);

        /**
            @brief	Sets up board routing for E2E.
        **/
        void			RouteE2ESignal (void);


	//	Private Member Data
	private:
        CNTV2Card				mDevice;			///< @brief	My CNTV2Card instance

};	//	NTV2MetalE2E

#endif	//	_NTV2MetalE2E_H
