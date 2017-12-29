/**
    @file		ntv2supportlogger.h
    @brief		Provides a simple way to log the status of NTV2 subsections
    @copyright	(C) 2017 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
**/

#ifndef NTV2SUPPORTLOGGER_H
#define NTV2SUPPORTLOGGER_H

#include "ntv2card.h"

typedef enum
{
    NTV2_SupportLoggerSectionAutoCirculate  = 0x00000001 << 0,
    NTV2_SupportLoggerSectionAudioLog       = 0x00000001 << 1,
    NTV2_SupportLoggerSectionRouting        = 0x00000001 << 2,
    NTV2_SupportLoggerSectionRegisterLog    = 0x00000001 << 3,
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

    void PrependCustomSection(const std::string& sectionName, const std::string& sectionData);

    void AppendCustomSection(const std::string& sectionName, const std::string& sectionData);

    std::string ToString();

    void ToString(std::string& outString);


private:
    void FetchRegisterLogInfo(std::ostringstream& oss);
    void FetchAutoCirculateLogInfo(std::ostringstream& oss);
    void FetchAudioLogInfo(std::ostringstream& oss);
    void FetchRoutingLogInfo(std::ostringstream& oss);

    CNTV2Card mDevice;
    NTV2SupportLoggerSections mSections;
    std::string mHeaderStr;
    std::string mFooterStr;
};

AJAExport std::ostream & operator << (std::ostream & outStream, const CNTV2SupportLogger & inData);

#endif // NTV2SUPPORTLOGGER_H
