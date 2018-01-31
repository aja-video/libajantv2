/**
    @file		ntv2supportlogger.h
    @brief		Provides a simple way to log the status of NTV2 subsections
    @copyright	(C) 2017 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
**/

#ifndef NTV2SUPPORTLOGGER_H
#define NTV2SUPPORTLOGGER_H

#include "ntv2card.h"
#include <map>
#include <string>

typedef enum
{
    NTV2_SupportLoggerSectionInfo           = 0x00000001 << 0,
    NTV2_SupportLoggerSectionAutoCirculate  = 0x00000001 << 1,
    NTV2_SupportLoggerSectionAudio          = 0x00000001 << 2,
    NTV2_SupportLoggerSectionRouting        = 0x00000001 << 3,
    NTV2_SupportLoggerSectionRegisters      = 0x00000001 << 4,
    NTV2_SupportLoggerSectionsAll           = 0xFFFFFFFF
} NTV2SupportLoggerSections;

class AJAExport CNTV2SupportLogger
{
public:

    CNTV2SupportLogger(CNTV2Card& card,
                       NTV2SupportLoggerSections sections = NTV2_SupportLoggerSectionsAll);

    CNTV2SupportLogger(int cardIndex = 0,
                       NTV2SupportLoggerSections sections = NTV2_SupportLoggerSectionsAll);

    virtual ~CNTV2SupportLogger();

    static int Version();

    void PrependToSection(uint32_t section, const std::string& sectionData);

    void AppendToSection(uint32_t section, const std::string& sectionData);

    void AddHeader(const std::string& sectionName, const std::string& sectionData);

    void AddFooter(const std::string& sectionName, const std::string& sectionData);

    std::string ToString();

    void ToString(std::string& outString);

	bool LoadFromLog (const std::string & inLogFilePath, const uint32_t inOptions = 0);


private:
    void FetchInfoLog(std::ostringstream& oss);
    void FetchRegisterLog(std::ostringstream& oss);
    void FetchAutoCirculateLog(std::ostringstream& oss);
    void FetchAudioLog(std::ostringstream& oss);
    void FetchRoutingLog(std::ostringstream& oss);

    CNTV2Card mDevice;
    NTV2SupportLoggerSections mSections;
    std::string mHeaderStr;
    std::string mFooterStr;

    std::map<uint32_t, std::string> mPrependMap;
    std::map<uint32_t, std::string> mAppendMap;
};

AJAExport std::ostream & operator << (std::ostream & outStream, const CNTV2SupportLogger & inData);

#endif // NTV2SUPPORTLOGGER_H
