/////////////////////////////////////////////////////////////////////////////
// ntv2spiinterface.h
//
// Copyright (C) 2017 AJA Video Systems, Inc.  Proprietary and Confidential information.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef NTV2SPIINTERFACE_H
#define NTV2SPIINTERFACE_H

#include "ntv2card.h"

class CNTV2SpiFlash
{
public:
    CNTV2SpiFlash() {}
    virtual ~CNTV2SpiFlash() {}

    virtual bool Read(const uint32_t address, std::vector<uint8_t> &data, uint32_t maxBytes = 1) = 0;
    virtual bool Write(const uint32_t address, const std::vector<uint8_t> data, uint32_t maxBytes = 1) = 0;
    virtual bool Erase(const uint32_t address, uint32_t bytes) = 0;
    virtual bool Verify(const uint32_t address, const std::vector<uint8_t>& dataWritten) = 0;
    virtual uint32_t Size() = 0;
    virtual void SetVerbosity(bool verbose) = 0;
    virtual bool GetVerbosity() = 0;

    static bool DeviceSupported(NTV2DeviceID deviceId) {(void)deviceId; return false;}
};

class CNTV2AxiSpiFlash : public CNTV2SpiFlash
{
public:
    CNTV2AxiSpiFlash(int index = 0, bool verbose = false);
    virtual ~CNTV2AxiSpiFlash();

    // common flash interface
    virtual bool Read(const uint32_t address, std::vector<uint8_t> &data, uint32_t maxBytes = 1);
    virtual bool Write(const uint32_t address, const std::vector<uint8_t> data, uint32_t maxBytes = 1);
    virtual bool Erase(const uint32_t address, uint32_t bytes);
    virtual bool Verify(const uint32_t address, const std::vector<uint8_t>& dataWritten);
    virtual uint32_t Size();
    void SetVerbosity(bool verbose);
    bool GetVerbosity();
    static bool DeviceSupported(NTV2DeviceID deviceId);

    // Axi specific

    // probably not leaving this, for testing
    bool ProgramFile(const std::string& sourceFile, const uint32_t fileStartOffset = 0, const uint32_t address = 0, const uint32_t maxBytes = 1, bool verify = true);
    bool DumpToFile(const std::string& outFile, const uint32_t flashStartOffset = 0, const uint32_t maxBytes = 1);
    bool WriteSerialAndMac(const std::string& serial, const uint32_t macOffset = 0x01F80000, const uint32_t serialOffset = 0x01FC0000);

private:
    bool NTV2DeviceOk();

    void SpiReset();
    bool SpiResetFifos();
    bool SpiWaitForWriteFifoEmpty();
    void SpiEnableWrite(bool enable);
    bool SpiTransfer(std::vector<uint8_t> commandSequence,
                     const std::vector<uint8_t> inputData,
                     std::vector<uint8_t>& outputData, uint32_t maxByteCutoff = 1);

    bool FlashDeviceInfo(uint8_t& manufactureID, uint8_t& memInerfaceType,
                         uint8_t& memDensity, uint8_t& sectorArchitecture,
                         uint8_t& familyID);
    bool FlashReadConfig(uint8_t& configValue);
    bool FlashReadStatus(uint8_t& statusValue);
    bool FlashReadBankAddress(uint8_t& bankAddressVal);
    bool FlashWriteBankAddress(const uint8_t bankAddressVal);

    void FlashFixAddress(const uint32_t address, std::vector<uint8_t>& commandSequence);

    bool        mVerbose;
    uint32_t    mBaseByteAddress;
    uint32_t    mSize;
    uint32_t    mSectorSize;

    CNTV2Card   mDevice;

    uint32_t    mSpiResetReg;
    uint32_t    mSpiControlReg;
    uint32_t    mSpiStatusReg;
    uint32_t    mSpiWriteReg;
    uint32_t    mSpiReadReg;
    uint32_t    mSpiSlaveReg;
    uint32_t    mSpiGlobalIntReg;
};

#endif
