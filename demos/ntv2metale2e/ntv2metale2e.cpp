/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2metale2e.cpp
	@brief		Implementation of NTV2OutputTestPattern demonstration class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2metale2e.h"

static void hex_to_bytes(char *hex, uint8_t *output, uint32_t array_length);
static bool spi_genlock2_write(struct ntv2_genlock2 *ntv2_gen, uint32_t size, uint16_t addr, uint8_t* data, bool triggerWait);
static bool spi_genlock2_read(struct ntv2_genlock2 *ntv2_gen, uint16_t addr, uint8_t* data, uint32_t numBytes);
static void spi_reset_fifos(struct ntv2_genlock2 *ntv2_gen);
static uint32_t reg_read(struct ntv2_genlock2 *ntv2_gen, const uint32_t *reg);
static void reg_write(struct ntv2_genlock2 *ntv2_gen, const uint32_t *reg, uint32_t data);

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
	
	//	Set up the E2E routing
	RouteE2ESignal();
	
	//  Set up the desired video configuration...
	status = SetUpVideo();
	if (AJA_FAILURE(status))
		return status;


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

    spi_reset(ntv2_gen);

    while ((gdat->size != 0))
    {
        //NTV2_MSG_GENLOCK_INFO("Writing offset %02X %s", gdat->offset, gdat->data);
        hex_to_bytes(gdat->data+2, writeBytes, gdat->size);
        if (!spi_genlock2_write(ntv2_gen, gdat->size, gdat->addr, writeBytes, true))
        {
            NTV2_MSG_ERROR("%s: genlock spi write failed\n", ntv2_gen->name);
            return false;
        }
        totalBytes += gdat->size;

        if (check)
        {
            uint8_t readBytes[256];
            uint16_t i;
            if (spi_genlock2_read(ntv2_gen, gdat->addr, readBytes, gdat->size))
            {
                for (i = 0; i < gdat->size; i++)
                {
                    spi_genlock2_read(ntv2_gen, gdat->addr+i, readBytes+i, 1);
                    if (readBytes[i] != writeBytes[i])
                    {
                        errorCount++;
                        NTV2_MSG_GENLOCK_INFO("%s: Set: %d, Offset: %04X, size: %d, data: %s", ntv2_gen->name, count, gdat->addr, gdat->size, gdat->data);
                        NTV2_MSG_GENLOCK_INFO("%s: Bytes did not match i : %d read : %02X write : %02X\n", ntv2_gen->name, i, readBytes[i], writeBytes[i]);
                    }
                    else
                    {
                        //NTV2_MSG_GENLOCK_INFO("%s: Bytes matched\n", ntv2_gen->name);
                    }
                }
            }
        }
        else
        {
            NTV2_MSG_GENLOCK_INFO("%s: Set: %d, Offset: %04X No check", ntv2_gen->name, count, gdat->addr);
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


// ---------------------------------------------
// Helpers for genlock configuration
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

static bool spi_genlock2_write(struct ntv2_genlock2 *ntv2_gen, uint32_t size, uint16_t addr, uint8_t* data, bool triggerWait)
{
    uint32_t controlVal;
    uint8_t   page_select_buffer[10];
    uint32_t i;

    //if (!spi_wait_write_empty(ntv2_gen)) return false;

    // Step 1 reset FIFOs
    spi_reset_fifos(ntv2_gen);

    if (addr != 0x7C)
    {
        // set page
        page_select_buffer[0] = addr & 0x80;
        page_select_buffer[1] = addr >> 8;
        page_select_buffer[2] = 0x10;
        page_select_buffer[3] = 0x20;

        //NTV2_MSG_GENLOCK_INFO("Write: Set Page");
        spi_genlock2_write(ntv2_gen, 4, 0x7C, page_select_buffer, true);
    }

    // Step 2 load data
    reg_write(ntv2_gen, ntv2_reg_spi_write, addr & 0x7f);
    //NTV2_MSG_GENLOCK_INFO("Wrote %02X", offset);
    for (i = 0; i < size; i++)
    {
        reg_write(ntv2_gen, ntv2_reg_spi_write, data[i]);
        //NTV2_MSG_GENLOCK_INFO("Wrote %02X", data[i]);
    }

    // Reset DRT Shift bit
    reset_dtr_status(ntv2_gen);

    // Step 3 chip select low
    reg_write(ntv2_gen, ntv2_reg_spi_slave, 0x0);

    // Step 4 enable master transactions
    controlVal = reg_read(ntv2_gen, ntv2_reg_spi_control);
    controlVal &= ~0x100;
    reg_write(ntv2_gen, ntv2_reg_spi_control, controlVal);

    spi_wait_write_empty(ntv2_gen);

    // Step 5 deassert chip select
    reg_write(ntv2_gen, ntv2_reg_spi_slave, 0x01);

    // Step 6 disable master transactions
    controlVal = reg_read(ntv2_gen, ntv2_reg_spi_control);
    controlVal |= 0x100;
    reg_write(ntv2_gen, ntv2_reg_spi_control, controlVal);

    if (triggerWait)
        wait_genlock2(ntv2_gen, 200);

    return true;
}

static bool spi_genlock2_read(struct ntv2_genlock2 *ntv2_gen, uint16_t addr, uint8_t* data, uint32_t numBytes)
{
    uint32_t  val, status;
    uint8_t   tx_buffer[10];
    uint32_t i;

    //if (!spi_wait_write_empty(ntv2_gen))
    //return false;

    // Step 1 reset FIFOs
    spi_reset_fifos(ntv2_gen);

    if (addr != 0x7c)
    {
        // set page
        tx_buffer[0] = addr & 0x80;
        tx_buffer[1] = addr >> 8;
        tx_buffer[2] = 0x10;
        tx_buffer[3] = 0x20;

        //NTV2_MSG_GENLOCK_INFO("Read: Set Page");
        spi_genlock2_write(ntv2_gen, 4, 0x7C, tx_buffer, true);
    }

    reg_write(ntv2_gen, ntv2_reg_spi_control, 0xfe);//0xe6);
    wait_genlock2(ntv2_gen, 1000);
    reg_write(ntv2_gen, ntv2_reg_spi_slave, 0x00);
    wait_genlock2(ntv2_gen, 1000);

    reg_write(ntv2_gen, ntv2_reg_spi_write, 0x80 | (addr & 0x7f));
    wait_genlock2(ntv2_gen, 1000);

    for (i = 0; i < numBytes; i++)
        reg_write(ntv2_gen, ntv2_reg_spi_write, 0);

    if (!spi_wait_write_empty(ntv2_gen))
        return false;

    reg_write(ntv2_gen, ntv2_reg_spi_slave, 0x01);
    wait_genlock2(ntv2_gen, 10000);

    if(!spi_wait_read_not_empty(ntv2_gen))
        return false;

    //wait_genlock2(ntv2_gen, 1000);

    val = reg_read(ntv2_gen, ntv2_reg_spi_read); // dummy read for address sent
    //NTV2_MSG_GENLOCK_INFO("Read: Val: %02X", val);
    for (i = 0; i < numBytes; i++)
    {
        status = reg_read(ntv2_gen, ntv2_reg_spi_status);
        if(!spi_wait_read_not_empty(ntv2_gen))
            return false;
        val = reg_read(ntv2_gen, ntv2_reg_spi_read);
        //NTV2_MSG_GENLOCK_INFO("Read: Val: %02X", val);
        data[i] = (uint8_t)val;
    }

    reg_write(ntv2_gen, ntv2_reg_spi_control, 0xfe);//0xe6);

    return true;
}

static void spi_reset_fifos(struct ntv2_genlock2 *ntv2_gen)
{
    //reg_write(ntv2_gen, ntv2_reg_spi_control, 0x1e6);
    reg_write(ntv2_gen, ntv2_reg_spi_control, 0x1fe);
}

static uint32_t reg_read(struct ntv2_genlock2 *ntv2_gen, const uint32_t *reg)
{
    return ntv2_reg_read(ntv2_gen->system_context, reg, ntv2_gen->index);
}

static void reg_write(struct ntv2_genlock2 *ntv2_gen, const uint32_t *reg, uint32_t data)
{
    ntv2_reg_write(ntv2_gen->system_context, reg, ntv2_gen->index, data);
}



