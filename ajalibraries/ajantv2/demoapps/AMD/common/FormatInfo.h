
#pragma once

#include <string>


namespace FormatInfo
{

    int     getFormat(const std::string strName);
    bool    getFormatName(int nFormat, std::string &ostrName);

    int     getInternalFormatSize(int nFormat);
    int     getInternalFormat(int idx);

    int     getExternalFormatSize(int nFormat, int nType);
    int     getExternalFormatComponents(int nFormat);
    int     getExternalFormat(int idx);

    int     getTypeSize(int nType);
    int     getType(const std::string strName);
    int     getType(int idx);
    bool    getTypeName(int nType, std::string &ostrName);

    int     getNumSupportedIntFormats();
    int     getNumSupportedExtFormats();
    int     getNumSupportedTypes();

    int     getAlignment(int nWidth, int nExtFormat, int nType);
}

