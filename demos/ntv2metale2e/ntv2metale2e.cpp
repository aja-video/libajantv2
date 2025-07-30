/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2metale2e.cpp
	@brief		Implementation of NTV2OutputTestPattern demonstration class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2metale2e.h"

void hex_to_bytes(char *hex, uint8_t *output, uint32_t array_length)
{
	uint32_t i;
	uint32_t j;
	
	for (i = 0, j = 0; i < array_length; i++, j+=2)
	{
		uint8_t bottom = hex[j+1] - (hex[j+1] > '9' ? 'A' - 10 : '0');
		uint8_t top = hex[j] - (hex[j] > '9' ? 'A' - 10 : '0');
		output[i] = (top * 16) + bottom;
	}
}

NTV2MetalE2E::NTV2MetalE2E ()
{
}	//	constructor


NTV2MetalE2E::~NTV2MetalE2E ()
{
}	//	destructor


AJAStatus NTV2MetalE2E::DoSomething (void)
{
    mDevice.Open(0);
	NTV2DeviceID mDeviceID = mDevice.GetDeviceID();	//	Keep this ID handy -- it's used frequently

    //if (mDeviceID != DEVICE_ID_KONAX)
        //return AJA_STATUS_FAIL;
	
	//	Set up the Genlock circuit
    AJAStatus status (SetUpGenlock());
		if (AJA_FAILURE(status))
            return status;
	
	//  Set up the desired video configuration...
	status = SetUpVideo();
	if (AJA_FAILURE(status))
		return status;
	
	//	Set up the E2E routing
	RouteE2ESignal();

	return AJA_STATUS_SUCCESS;

}	//	Init

AJAStatus NTV2MetalE2E::SetUpGenlock (void)
{
	//  Port driver genlock

    struct ntv2_genlock2_data* gdat = s_rc32012a_broadcast_1485;
    uint32_t count = 0, errorCount = 0, totalBytes = 0;
    uint32_t value;
    uint32_t mask;
    uint8_t writeBytes[256];
    uint32_t outFreq1 = 0, outFreq2 = 0, outFreq3 = 0, outFreq4 = 0, outFreq5 = 0;
    uint8_t dpll0Status[1] = { 0 };
    uint32_t gpioValue;
	bool check = false;

    SPIReset();

    while ((gdat->size != 0))
    {
        //ntv2Message("Writing offset %02X %s", gdat->addr, gdat->data);
        hex_to_bytes(gdat->data+2, writeBytes, gdat->size);
        if (!SPIGenlock2Write(gdat->size, gdat->addr, writeBytes, true))
        {
            //ntv2Message("genlock spi write failed\n");
            return AJA_STATUS_FAIL;
        }
        totalBytes += gdat->size;

        if (check)
        {
            uint8_t readBytes[256];
            uint16_t i;
            if (SPIGenlock2Read(gdat->addr, readBytes, gdat->size))
            {
                for (i = 0; i < gdat->size; i++)
                {
                    SPIGenlock2Read(gdat->addr+i, readBytes+i, 1);
                    if (readBytes[i] != writeBytes[i])
                    {
                        errorCount++;
                        ntv2Message("Set: %d, Offset: %04X, size: %d, data: %s", count, gdat->addr, gdat->size, gdat->data);
                        ntv2Message("Bytes did not match i : %d read : %02X write : %02X\n", i, readBytes[i], writeBytes[i]);
                    }
                    else
                    {
                        //ntv2Message("Bytes matched\n");
                    }
                }
            }
        }
        else
        {
            ntv2Message("Set: %d, Offset: %04X No check", count, gdat->addr);
        }
        count++;
        gdat++;
    }


    return AJA_STATUS_SUCCESS;
	
}

AJAStatus NTV2MetalE2E::SetUpVideo (void)
{
	mDevice.SetReference(NTV2_REFERENCE_INPUT1);
	mDevice.EnableChannel(NTV2_CHANNEL1);
	mDevice.EnableChannel(NTV2_CHANNEL2);
	
	NTV2VideoFormat videoFormat = mDevice.GetInputVideoFormat (NTV2_INPUTSOURCE_SDI1);
	mDevice.SetVideoFormat (videoFormat, false, false, NTV2_CHANNEL1);
	mDevice.SetVideoFormat (videoFormat, false, false, NTV2_CHANNEL2);
	
	NTV2Standard videoStandard = GetNTV2StandardFromVideoFormat(videoFormat);
	mDevice.SetSDIOutputStandard(NTV2_CHANNEL2, videoStandard);
	
    bool isValidVPID (mDevice.GetVPIDValidA(NTV2_CHANNEL1));
	if (isValidVPID)
	{
		CNTV2VPID inputVPID;
		ULWord vpidDS1(0), vpidDS2(0);
		mDevice.ReadSDIInVPID(NTV2_CHANNEL1, vpidDS1, vpidDS2);
		inputVPID.SetVPID(vpidDS1);
		isValidVPID = inputVPID.IsValid();
		mDevice.SetSDIOutVPID(vpidDS1, vpidDS2, NTV2_CHANNEL2);
	}
	
    bool is3G (false), is6G (false), is12G (false);
	mDevice.GetSDIInput3GPresent(is3G, NTV2_CHANNEL1);
	mDevice.GetSDIInput6GPresent(is6G, NTV2_CHANNEL1);
	mDevice.GetSDIInput12GPresent(is12G, NTV2_CHANNEL1);
	
	mDevice.SetSDIOut3GEnable(NTV2_CHANNEL2, is3G);
	mDevice.SetSDIOut6GEnable(NTV2_CHANNEL2, is6G);
	mDevice.SetSDIOut12GEnable(NTV2_CHANNEL2, is12G);
	
	return AJA_STATUS_SUCCESS;
	
}	//	SetUpVideo

void NTV2MetalE2E::RouteE2ESignal (void)
{
    mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, false);
    mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);
    mDevice.ClearRouting();
    mDevice.Connect(NTV2_XptSDIOut2Input, NTV2_XptSDIIn1);

}	//	RouteOutputSignal

bool NTV2MetalE2E::SPIWaitWriteEmpty (void)
{
	uint32_t dtrStatus = 0;
	uint32_t count = 0;
	
	/*status = RegRead(ntv2_reg_spi_status);
    while ((status & GENL_SPI_WRITE_FIFO_EMPTY) == 0)
	{
		if (count++ > c_spi_timeout) return false;
		status = RegRead(ntv2_reg_spi_status);
		ntv2Message("FIFO NOT EMPTY!\n");
	}
*/
	dtrStatus = RegRead(ntv2_reg_spi_ip_status);
	count = 0;
	while ((dtrStatus & DTR_EMPTY) == 0)
	{
		if (count++ > c_spi_timeout) return false;
		dtrStatus = RegRead(ntv2_reg_spi_ip_status);
		//ntv2Message("DTR NOT EMPTY!\n");
	}
	
	//ntv2Message("Transfer Complete! %d\n", count);
	return true;
}

bool NTV2MetalE2E::SPIWaitReadNotEmpty (void)
{
	uint32_t status = 0;
	uint32_t count = 0;
	
	status = RegRead(ntv2_reg_spi_status);
	while ((status & GENL_SPI_READ_FIFO_EMPTY) != 0)
	{
		if (count++ > c_spi_timeout) return false;
		status = RegRead(ntv2_reg_spi_status);
		//ntv2Message("READ FIFO EMPTY!");
	}
	//ntv2Message("READ FIFO NOT EMPTY! %d", count);
	return true;
}

bool NTV2MetalE2E::ResetDTRStatus (void)
{
	uint32_t dtrStatus = RegRead(ntv2_reg_spi_ip_status);
	if ((dtrStatus & DTR_EMPTY) != 0)
	{
		//ntv2Message("DTR High!\n");
		mDevice.WriteRegister(ntv2_reg_spi_ip_status, 1, BIT(2), 2);
		//dtrStatus = RegRead(ntv2_reg_spi_ip_status);
		//ntv2Message("DTR Reset Value: %08X!\n", dtrStatus);
		return true;
	}
	return false;
}

bool NTV2MetalE2E::SPIGenlock2Write (uint32_t size, uint16_t addr, uint8_t* data, bool triggerWait)
{
    uint32_t controlVal;
    uint8_t   page_select_buffer[10];
    uint32_t i;

    //if (!SPIWaitWriteEmpty(ntv2_gen)) return false;

    // Step 1 reset FIFOs
    SPIResetFIFOs();

    if (addr != 0x7C)
    {
        // set page
        page_select_buffer[0] = addr & 0x80;
        page_select_buffer[1] = addr >> 8;
        page_select_buffer[2] = 0x10;
        page_select_buffer[3] = 0x20;

        //ntv2Message("Write: Set Page");
        SPIGenlock2Write(4, 0x7C, page_select_buffer, true);
    }

    // Step 2 load data
    mDevice.WriteRegister(ntv2_reg_spi_write, addr & 0x7f);
    //ntv2message("Wrote %02X", offset);
    for (i = 0; i < size; i++)
    {
        mDevice.WriteRegister(ntv2_reg_spi_write, data[i]);
        //ntv2Message("Wrote %02X", data[i]);
    }

    // Reset DRT Shift bit
    ResetDTRStatus();

    // Step 3 chip select low
    mDevice.WriteRegister(ntv2_reg_spi_slave, 0x0);

    // Step 4 enable master transactions
    controlVal = RegRead(ntv2_reg_spi_control);
    controlVal &= ~0x100;
    mDevice.WriteRegister(ntv2_reg_spi_control, controlVal);

    SPIWaitWriteEmpty();

    // Step 5 deassert chip select
    mDevice.WriteRegister(ntv2_reg_spi_slave, 0x01);

    // Step 6 disable master transactions
    controlVal = RegRead(ntv2_reg_spi_control);
    controlVal |= 0x100;
    mDevice.WriteRegister(ntv2_reg_spi_control, controlVal);

    if (triggerWait)
        WaitGenlock2(200);

    return true;
}

bool NTV2MetalE2E::SPIGenlock2Read (uint16_t addr, uint8_t* data, uint32_t numBytes)
{
    uint32_t  val, status;
    uint8_t   tx_buffer[10];
    uint32_t i;

    //if (!SPIWaitWriteEmpty())
    //return false;

    // Step 1 reset FIFOs
    SPIResetFIFOs();

    if (addr != 0x7c)
    {
        // set page
        tx_buffer[0] = addr & 0x80;
        tx_buffer[1] = addr >> 8;
        tx_buffer[2] = 0x10;
        tx_buffer[3] = 0x20;

        //ntv2Message("Read: Set Page");
        SPIGenlock2Write(4, 0x7C, tx_buffer, true);
    }

    mDevice.WriteRegister(ntv2_reg_spi_control, 0xfe);
    WaitGenlock2(1000);
    mDevice.WriteRegister(ntv2_reg_spi_slave, 0x00);
    WaitGenlock2(1000);

    mDevice.WriteRegister(ntv2_reg_spi_write, 0x80 | (addr & 0x7f));
    WaitGenlock2(1000);

    for (i = 0; i < numBytes; i++)
        mDevice.WriteRegister(ntv2_reg_spi_write, 0);

    if (!SPIWaitWriteEmpty())
        return false;

    mDevice.WriteRegister(ntv2_reg_spi_slave, 0x01);
    WaitGenlock2(10000);

    if(!SPIWaitReadNotEmpty())
        return false;

    //wait_genlock2(1000);

    val = RegRead(ntv2_reg_spi_read); // dummy read for address sent
    //ntv2Message("Read: Val: %02X", val);
    for (i = 0; i < numBytes; i++)
    {
        status = RegRead(ntv2_reg_spi_status);
        if(!SPIWaitReadNotEmpty())
            return false;
        val = RegRead(ntv2_reg_spi_read);
        //ntv2Message("Read: Val: %02X", val);
        data[i] = (uint8_t)val;
    }

    mDevice.WriteRegister(ntv2_reg_spi_control, 0xfe);//0xe6);

    return true;
}

bool NTV2MetalE2E::WaitGenlock2 (uint32_t numMicrosSeconds)
{
	uint32_t usTicks = 0;
	uint32_t timeoutCount = 0;
	
	mDevice.WriteRegister(0x3606, 0);
	while (usTicks < numMicrosSeconds)
	{
		usTicks = RegRead(0x3606);
#if 1
		if(timeoutCount++ > 100) return false;
#else
		if (timeoutCount++ > 100)
		{
			ntv2Message("Genlock2 200us wait TIMEDOUT");
			return false;
		}
		else
		{
			ntv2Message("Genlock2 timeout count: %d", timeoutCount);
			return true;
		}
#endif
	}
	return true;
}

void NTV2MetalE2E::SPIReset (void)
{	
	// reset spi hardware
	mDevice.WriteRegister(ntv2_reg_spi_reset, 0x0a);
	
	// configure spi & reset fifos
	mDevice.WriteRegister(ntv2_reg_spi_slave, 0x1);
	mDevice.WriteRegister(ntv2_reg_spi_control, 0x1fe);
}

void NTV2MetalE2E::SPIResetFIFOs (void)
{
    mDevice.WriteRegister(ntv2_reg_spi_control, 0x1fe);
}

uint32_t NTV2MetalE2E::RegRead (uint32_t reg)
{
	uint32_t outVal(0);
	mDevice.ReadRegister(reg, outVal);
	return outVal;
}



