/**
    @file		main.cpp
    @brief		Unittests for the AJA Base Library (using doctest).
    @copyright	Copyright (c) 2017 AJA Video Systems, Inc. All rights reserved.
**/
// for doctest usage see: https://github.com/onqtam/doctest/blob/1.1.4/doc/markdown/tutorial.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "limits.h"

#include "ntv2card.h"
#include "ntv2spiinterface.h"
#include "ajabase/common/common.h"
#include "ajabase/system/systemtime.h"

#if 0
template
void filename_marker() {} //this is used to easily just around in a GUI with a symbols list
TEST_SUITE("filename -- functions in streams/common/filename.h");

    TEST_CASE("constructor")
    {
    }

TEST_SUITE_END(); //filename
#endif





void spi_marker() {}
TEST_SUITE("spi -- new spi flasher");

    TEST_CASE("normal")
    {
        int testData[2];
        testData[0] = 0x12345678;
        testData[1] = 0x89ABCDEF;
        std::ofstream testFile("test.bit", std::ofstream::binary);
        testFile.write((char*)testData, sizeof(testData));
        testFile.close();

        SPIProgramErrors result;

        // test writing out 8 bytes
        CNTV2SPI spi1(2, 4, 0x4);
        result = spi1.ProgramFile("test.bit", 0, -1, 0, 0, true, true);
        CHECK(result == SPI_Ok);
        CHECK(spi1.GetFlashAsString(0) == "78563412EFCDAB890000000000000000");
        CHECK(spi1.GetFlashAsString(1) == "00000000000000000000000000000000");
        std::vector<bool> dirtyList;
        std::vector<bool> dirtyTruth({true, true, false, false});
        spi1.GetBankDirtyList(0, dirtyList);
        CHECK(std::equal(dirtyList.begin(), dirtyList.end(), dirtyTruth.begin()));

        // test limiting to 2 bytes even if 8 available
        CNTV2SPI spi2(2, 4, 0x4);
        result = spi2.ProgramFile("test.bit", 0, 2, 0, 0, true, true);
        CHECK(result == SPI_Ok);
        CHECK(spi2.GetFlashAsString(0) == "78560000000000000000000000000000");
        CHECK(spi2.GetFlashAsString(1) == "00000000000000000000000000000000");
        std::vector<bool> dirtyList2;
        std::vector<bool> dirtyTruth2({true, false, false, false});
        spi2.GetBankDirtyList(0, dirtyList2);
        CHECK(std::equal(dirtyList2.begin(), dirtyList2.end(), dirtyTruth2.begin()));

        // test starting at offset 2
        CNTV2SPI spi3(2, 4, 0x4);
        result = spi3.ProgramFile("test.bit", 2, -1, 0, 0, true, true);
        CHECK(result == SPI_Ok);
        CHECK(spi3.GetFlashAsString(0) == "3412EFCDAB8900000000000000000000");
        CHECK(spi3.GetFlashAsString(1) == "00000000000000000000000000000000");
        std::vector<bool> dirtyList3;
        std::vector<bool> dirtyTruth3({true, true, false, false});
        spi3.GetBankDirtyList(0, dirtyList3);
        CHECK(std::equal(dirtyList3.begin(), dirtyList3.end(), dirtyTruth3.begin()));

        // test failing if offset after num of bytes
        CNTV2SPI spi4(2, 4, 0x4);
        result = spi4.ProgramFile("test.bit", 10, -1, 0, 0, true, true);
        CHECK(result == SPI_ReadingOutsideFileBounds);
        CHECK(spi4.GetFlashAsString(0) == "00000000000000000000000000000000");
        CHECK(spi4.GetFlashAsString(1) == "00000000000000000000000000000000");
        std::vector<bool> dirtyList4;
        std::vector<bool> dirtyTruth4({false, false, false, false});
        spi4.GetBankDirtyList(0, dirtyList4);
        CHECK(std::equal(dirtyList4.begin(), dirtyList4.end(), dirtyTruth4.begin()));

        // test failing if bank index out of bounds
        CNTV2SPI spi5(2, 4, 0x4);
        result = spi5.ProgramFile("test.bit", 0, -1, 5, 0, true, true);
        CHECK(result == SPI_AccessingInvalidBank);
        CHECK(spi5.GetFlashAsString(0) == "00000000000000000000000000000000");
        CHECK(spi5.GetFlashAsString(1) == "00000000000000000000000000000000");
        std::vector<bool> dirtyList5;
        std::vector<bool> dirtyTruth5({false, false, false, false});
        spi5.GetBankDirtyList(0, dirtyList5);
        CHECK(std::equal(dirtyList5.begin(), dirtyList5.end(), dirtyTruth5.begin()));

        // test failing if flash chip index out of bounds
        CNTV2SPI spi6(2, 4, 0x4);
        result = spi6.ProgramFile("test.bit", 0, -1, 0, 3, true, true);
        CHECK(result == SPI_AccessingInvalidFlashChip);
        CHECK(spi6.GetFlashAsString(0) == "00000000000000000000000000000000");
        CHECK(spi6.GetFlashAsString(1) == "00000000000000000000000000000000");
        std::vector<bool> dirtyList6;
        std::vector<bool> dirtyTruth6({false, false, false, false});
        spi6.GetBankDirtyList(0, dirtyList6);
        CHECK(std::equal(dirtyList6.begin(), dirtyList6.end(), dirtyTruth6.begin()));

        // test trying to write too much data to flash
        CNTV2SPI spi7(1, 1, 0x4);
        result = spi7.ProgramFile("test.bit", 0, -1, 0, 0, true, true);
        CHECK(result == SPI_FileBiggerThanAllFlash);
        CHECK(spi7.GetFlashAsString(0) == "00000000");
        std::vector<bool> dirtyList7;
        std::vector<bool> dirtyTruth7({false});
        spi7.GetBankDirtyList(0, dirtyList7);
        CHECK(std::equal(dirtyList7.begin(), dirtyList7.end(), dirtyTruth7.begin()));

        // test trying to write too much data to 1 bank
        CNTV2SPI spi8(1, 1, 0x4);
        result = spi8.ProgramFile("test.bit", 0, 5, 0, 0, false, false);
        CHECK(result == SPI_FileSpansMoreThanOneBank);
        CHECK(spi8.GetFlashAsString(0) == "00000000");
        std::vector<bool> dirtyList8;
        std::vector<bool> dirtyTruth8({false});
        spi8.GetBankDirtyList(0, dirtyList8);
        CHECK(std::equal(dirtyList8.begin(), dirtyList8.end(), dirtyTruth8.begin()));

        // test trying to write too much data to 1 flash
        CNTV2SPI spi9(2, 1, 0x4);
        result = spi9.ProgramFile("test.bit", 0, 5, 0, 0, true, false);
        CHECK(result == SPI_FileSpansMoreThanOneChip);
        CHECK(spi9.GetFlashAsString(0) == "00000000");
        std::vector<bool> dirtyList9;
        std::vector<bool> dirtyTruth9({false});
        spi9.GetBankDirtyList(0, dirtyList9);
        CHECK(std::equal(dirtyList9.begin(), dirtyList9.end(), dirtyTruth9.begin()));

        CNTV2SPI spi10(DEVICE_ID_IOIP_2022,0);
        spi10.ResetSPIInterface();
        uint8_t manufactureID = 0;
        uint8_t memInerfaceType = 0;
        uint8_t memDensity = 0;
        uint8_t sectorArchitecture = 0;
        uint8_t familyID = 0;
        bool readDeviceGood = spi10.SPIReadDeviceID(manufactureID, memInerfaceType,
                                                    memDensity, sectorArchitecture,
                                                    familyID);
        CHECK(readDeviceGood == true);
        CHECK(manufactureID == 0x1);
        CHECK(memInerfaceType == 0x02);
        CHECK(memDensity == 0x19);
        CHECK(sectorArchitecture == 0x01);
        CHECK(familyID == 0x80);

        /*
        uint8_t bankAddressValue = 0;
        bool readBankGood = spi10.SPIReadBankAddress(bankAddressValue);
        CHECK(bankAddressValue == 0);
        bool writeBankGood = spi10.SPIWriteBankAddress(0x2);
        uint8_t bankAddressValue2 = 0;
        readBankGood = spi10.SPIReadBankAddress(bankAddressValue2);
        CHECK(bankAddressValue2 == 0x2);
        spi10.SPIWriteBankAddress(bankAddressValue); // rest back
        */

        /*
        uint32_t addr = 0;
        spi10.SPIEraseSector(0, 1);

        std::vector<uint8_t> writeData;
        for(int i=0;i<8;i++)
        {
            writeData.push_back(0x2A);
            writeData.push_back(0x55);
        }
        spi10.SPIWrite(addr, writeData, 16);

        std::vector<uint8_t> data;
        spi10.SPIRead(addr, data, 10);
        CHECK(data.size() == 10);
        CHECK(data.at(0) == 0x2A);
        */

        CNTV2AxiSpiFlash spiNew(0);
        uint32_t flashSize = spiNew.Size();
        CHECK(flashSize == 0x2000000);

        uint32_t addr = 0x20000;

        AJATime::Sleep(20);

        std::vector<uint8_t> dataBeforeErase;
        spiNew.Read(addr, dataBeforeErase, 10);
        CHECK(dataBeforeErase.size() == 10);

        spiNew.Erase(addr, 10);

        AJATime::Sleep(20);

        std::vector<uint8_t> dataAfterErase;
        spiNew.Read(addr, dataAfterErase, 10);
        CHECK(dataAfterErase.size() == 10);

        AJATime::Sleep(20);

        std::vector<uint8_t> writeData;
        for(int i=0;i<5;i++)
        {
            writeData.push_back(0x2A);
            writeData.push_back(0x55);
        }
        spiNew.Write(addr, writeData, 10);

        AJATime::Sleep(20);

        std::vector<uint8_t> dataAfterWrite;
        spiNew.Read(addr, dataAfterWrite, 10);
        CHECK(dataAfterWrite.size() == 10);
        /*
        spiNew.Erase(addr, 16);

        std::vector<uint8_t> dataAfterErase;
        spiNew.Read(addr, dataAfterErase, 10);
        CHECK(dataAfterErase.size() == 10);

        std::vector<uint8_t> writeData;
        for(int i=0;i<8;i++)
        {
            writeData.push_back(0x2A);
            writeData.push_back(0x55);
        }
        spiNew.Write(addr, writeData, 16);

        std::vector<uint8_t> dataAfterWrite;
        spiNew.Read(addr, dataAfterWrite, 16);
        CHECK(dataAfterWrite.size() == 16);
        */
    }

TEST_SUITE_END(); //spi
