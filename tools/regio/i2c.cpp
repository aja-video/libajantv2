/* SPDX-License-Identifier: MIT */
/**
	@file		crossplatform/regio/i2c.cpp
	@brief		Implements helper functions for I2C communication.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.
**/

#include "i2c.h"

#if defined (AJALinux) || defined (AJAMac)
	#define I2CSleep(x) usleep(x)
#else
	#define I2CSleep(x) Sleep(x)
#endif

/*****************************************************************************************
 *	I2CWriteDataSingle
 *****************************************************************************************/

bool I2CWriteDataSingle(CNTV2Card* pCard, UByte I2CAddress, UByte I2CData)
{
	ULWord I2CCommand = I2CAddress;
	bool retVal;

	if (!I2CInhibitHardwareReads(pCard))
	{
		retVal = false;
		goto bail;
	}
	I2CSleep(1);

	I2CCommand <<= 8;
	I2CCommand |= I2CData;
	
	retVal = I2CWriteData(pCard, I2CCommand);

bail:
	if(!I2CEnableHardwareReads(pCard))
	{
		retVal = false;
	}

	return retVal;
}


/*****************************************************************************************
 *	I2CInhibitHardwareReads
 *****************************************************************************************/

bool I2CInhibitHardwareReads(CNTV2Card* pCard)
{
	bool retVal = true;

	if(!I2CWriteControl(pCard, BIT(16)))		// Inhibit I2C status register reads at vertical
	{		 
		retVal = false;
		I2CWriteControl(pCard, 0);			// Enable I2C status register reads at vertical
	}

	return retVal;
}


/*****************************************************************************************
 *	I2CEnableHardwareReads
 *****************************************************************************************/

bool I2CEnableHardwareReads(CNTV2Card* pCard)
{
	bool retVal = true;

	if(!I2CWriteControl(pCard, 0))		// Inhibit I2C status register reads at vertical
	{		 
		retVal = false;
	}

	return retVal;
}


/*****************************************************************************************
 *	I2CWriteDataDoublet
 *****************************************************************************************/

bool I2CWriteDataDoublet(CNTV2Card* pCard,
						UByte I2CAddress1, UByte I2CData1,
						UByte I2CAddress2, UByte I2CData2)
{
	bool retVal = true;
	ULWord I2CCommand;

	if (!I2CInhibitHardwareReads(pCard))
	{
		retVal = false;
		goto bail;
	}
	I2CSleep(1);

	I2CCommand = I2CAddress1;
	I2CCommand <<= 8;
	I2CCommand |= I2CData1;
	if (!I2CWriteData(pCard, I2CCommand))
	{		 
		retVal = false;
		goto bail;
	}

	I2CCommand = I2CAddress2;
	I2CCommand <<= 8;
	I2CCommand |= I2CData2;
	if(!I2CWriteData(pCard, I2CCommand))
	{
		retVal = false;
		goto bail;
	}

bail:
	if(!I2CEnableHardwareReads(pCard))
		retVal = false;

	return retVal;
}


/*****************************************************************************************
 *	WaitForI2CReady
 *****************************************************************************************/

bool WaitForI2CReady(CNTV2Card* pCard)
{
	ULWord registerValue;
	
	int iMSecs = 0;
	do
	{
		pCard->ReadRegister(kRegI2CWriteControl, registerValue);
		if ((registerValue & BIT(31)) == 0)
		{
			return true;
		}
		I2CSleep(1);
	} while (++iMSecs < 10);

	return false;
}


/*****************************************************************************************
 *	I2CWriteData
 *****************************************************************************************/

bool I2CWriteData(CNTV2Card* pCard, ULWord value)
{
	// wait for the hardware to be ready
	if (WaitForI2CReady(pCard) == false)
	{
		return false;
	}
	
	// write the data to the hardware
	pCard->WriteRegister(kRegI2CWriteData, value);
	I2CSleep(1);

	// wait for the I2C to complete writing
	if (WaitForI2CReady(pCard) == false) 
	{
		return false;
	}
	
	return true;
}


/*****************************************************************************************
 *	I2CWriteControl
 *****************************************************************************************/

bool I2CWriteControl(CNTV2Card* pCard, ULWord value)
{
	pCard->WriteRegister(kRegI2CWriteControl, value);
	return true;
}
