/* SPDX-License-Identifier: MIT */
/**
	@file		crossplatform/regio/i2c.h
	@brief		Declares helper functions for I2C communication.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.
**/

#ifndef I2C_H
#define I2C_H

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2publicinterface.h"
#include "ntv2card.h"

bool I2CWriteDataSingle(CNTV2Card* pCard, UByte I2CAddress, UByte I2CData);
bool I2CInhibitHardwareReads(CNTV2Card* pCard);
bool I2CEnableHardwareReads(CNTV2Card* pCard);
bool I2CWriteDataDoublet(CNTV2Card pCard,
						 UByte I2CAddress1, UByte I2CData1,
						 UByte I2CAddress2, UByte I2CData2);
bool WaitForI2CReady(CNTV2Card* pCard);
bool I2CWriteData(CNTV2Card* pCard, ULWord value);
bool I2CWriteControl(CNTV2Card* pCard, ULWord value);

#endif
