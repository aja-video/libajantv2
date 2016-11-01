
#include <string>
#include "GL/glew.h"

#include "FormatInfo.h"


namespace FormatInfo
{

    using namespace std;


    struct FormatInfo 
    {
        int          nFormat;
        int          nSize;
        const string strName; 
    };

    struct  TypeInfo
    {
        int         nType;
        int         nNeedsNumCompFormat;
        int         nSizeInByte;
        const string strName;
    };


    // Define supported internal formats. The size does not depend 
    // on additional parameters. The size parameter stores the size
    // of a pixel in bytes.
    // Only definite formats are supported here (RGBA8 and not RGBA)
    struct FormatInfo SupportedInternalFormats[] = 
    {
        // Format       Bytes Per Pixel     Name
        {GL_ALPHA16,    2,                  "GL_ALPHA16"},
        {GL_RGBA4,      2,                  "GL_RGBA4"},
        {GL_RGB4,       2,                  "GL_RGB4"},             // Even if the format uses only 12 Bit, 16 Bit will be transferred
        {GL_RGBA8,      4,                  "GL_RGBA8"},
        {GL_RGB8,       3,                  "GL_RGB8"},
        {GL_RGB10,      4,                  "GL_RGB10"},            // Even if the format uses only 30 Bit, 32 Bit will be transferred
        {GL_RGB16,      6,                  "GL_RGB16"},
        {GL_RGB10_A2,   4,                  "GL_RGB10_A2"},
        {GL_RGBA12,     6,                  "GL_RGBA12"},
        {GL_RGBA16,     8,                  "GL_RGBA16"},
        {GL_RGB32F,     12,                 "GL_RGB32F"},
        {GL_RGBA32F,    16,                 "GL_RGBA32F"},
        {GL_RGB16F,     6,                  "GL_RGB16F"},
        {GL_RGBA16F,    8,                  "GL_RGBA16F"}
    };


    // Define supported external formats. The size of of a pixel
    // depends on the format and type. The table below stores the
    // number of components in the size parameter.
    struct FormatInfo SupportedExternalFormats[] =
    {
        // Format   Num Comp    Name
        {GL_ALPHA,  1,          "GL_ALPHA"},
        {GL_RGB,    3,          "GL_RGB"},
        {GL_RGBA,   4,          "GL_RGBA"},
        {GL_BGR,    3,          "GL_BGR"},
        {GL_BGRA,   4,          "GL_BGRA"}
    };


    // The table below stores the supported types. Some types need a defined number
    // of components this value is stored in the second column. If the value is 0
    // no format constraints exist.
    // Size indicates either the size in bytes per component or the total size.
    struct TypeInfo SupportedTypes[] =
    {
        // Format                   Need #Comp    Size  Name
        {GL_UNSIGNED_BYTE,              0,          1,  "GL_UNSIGNED_BYTE"},
        {GL_UNSIGNED_SHORT,             0,          2,  "GL_UNSIGNED_SHORT"},
        {GL_UNSIGNED_INT,               0,          4,  "GL_UNSIGNED_INT"},
        {GL_FLOAT,                      0,          4,  "GL_FLOAT"},
        {GL_HALF_FLOAT,                 0,          2,  "GL_HALF_FLOAT"},
        {GL_UNSIGNED_SHORT_4_4_4_4,     4,          2,  "GL_UNSIGNED_SHORT_4_4_4_4"},
        {GL_UNSIGNED_INT_8_8_8_8,       4,          4,  "GL_UNSIGNED_INT_8_8_8_8"},
        {GL_UNSIGNED_INT_10_10_10_2,    4,          4,  "GL_UNSIGNED_INT_10_10_10_2"}
    };



    int getFormat(const string strName)
    {
        int i;
        int nNumIntFormats = sizeof(SupportedInternalFormats) / sizeof(SupportedInternalFormats[0]);
        int nNumExtFormats = sizeof(SupportedExternalFormats) / sizeof(SupportedExternalFormats[0]);

        for (i = 0; i < nNumExtFormats; i++)
        {
            if (SupportedExternalFormats[i].strName == strName)
                return SupportedExternalFormats[i].nFormat;
        }

        for (i = 0; i < nNumIntFormats; i++)
        {
            if (SupportedInternalFormats[i].strName == strName)
                return SupportedInternalFormats[i].nFormat;
        }
            
        return -1;
    }


    bool getFormatName(int nFormat, string &ostrName)
    {
        int i;
        int nNumIntFormats = sizeof(SupportedInternalFormats) / sizeof(SupportedInternalFormats[0]);
        int nNumExtFormats = sizeof(SupportedExternalFormats) / sizeof(SupportedExternalFormats[0]);

        for (i = 0; i < nNumExtFormats; i++)
        {
            if (SupportedExternalFormats[i].nFormat == nFormat)
            {
                ostrName = SupportedExternalFormats[i].strName;
                return true;
            }
        }

        for (i = 0; i < nNumIntFormats; i++)
        {
            if (SupportedInternalFormats[i].nFormat == nFormat)
            {
                ostrName = SupportedInternalFormats[i].strName;
                return true;
            }
        }
            
        return false;
    }


    // returns format size in bytes
    int getInternalFormatSize(int nFormat)
    {
        int i;
        int nNumIntFormats = sizeof(SupportedInternalFormats) / sizeof(SupportedInternalFormats[0]);

        for (i = 0; i < nNumIntFormats; i++)
        {
            if (SupportedInternalFormats[i].nFormat == nFormat)
            {
                return SupportedInternalFormats[i].nSize;
            }
        }

        return 0;
    }


    int getInternalFormat(int idx)
    {
        if (idx < getNumSupportedIntFormats())
            return SupportedInternalFormats[idx].nFormat;

        return 0;
    }


    int getNumSupportedIntFormats()
    {
        return sizeof(SupportedInternalFormats) / sizeof(SupportedInternalFormats[0]);
    }


    int getExternalFormatSize(int nFormat, int nType)
    {
        int i;
        int nNumComps = 0;
        int nNumExtFormats = sizeof(SupportedExternalFormats) / sizeof(SupportedExternalFormats[0]);
        int nNumTypes      = sizeof(SupportedTypes) / sizeof(SupportedTypes[0]);

        for (i = 0; i < nNumExtFormats; i++)
        {
            if (SupportedExternalFormats[i].nFormat == nFormat)
            {
                nNumComps = SupportedExternalFormats[i].nSize;
            }
        }

        for (i = 0; i < nNumTypes; i++)
        {
            if (SupportedTypes[i].nType == nType)
            {
                if (SupportedTypes[i].nNeedsNumCompFormat > 0 && SupportedTypes[i].nNeedsNumCompFormat == nNumComps)
                {
                    // this type requires a matching format (e.g. GL_UNSIGNED_INT_10_10_10_2 requires a 4 Comp format)
                    // In this case the size of the type is the size of one pixel
                    return SupportedTypes[i].nSizeInByte;
                }
                else if (SupportedTypes[i].nNeedsNumCompFormat == 0)
                {
                    // this type defines only the size of one component of the format
                    return (SupportedTypes[i].nSizeInByte * nNumComps);
                }
                    
            }
        }

        return 0;
    }

    int getExternalFormatComponents(int nFormat)
    {
        int nNumExtFormats = sizeof(SupportedExternalFormats) / sizeof(SupportedExternalFormats[0]);

        for (int i = 0; i < nNumExtFormats; i++)
        {
            if (SupportedExternalFormats[i].nFormat == nFormat)
            {
                return SupportedExternalFormats[i].nSize;
            }
        }

        return 0;
    }


    int getExternalFormat(int idx)
    {
        if (idx < getNumSupportedExtFormats())
            return SupportedExternalFormats[idx].nFormat;

        return 0;
    }


    int getNumSupportedExtFormats()
    {
        return sizeof(SupportedExternalFormats) / sizeof(SupportedExternalFormats[0]);
    }


    int getType(const std::string strName)
    {
        int nNumTypes = sizeof(SupportedTypes) / sizeof(SupportedTypes[0]);

        for (int i = 0; i < nNumTypes; i++)
        {
            if (SupportedTypes[i].strName == strName)
            {
                return SupportedTypes[i].nType;
            }
        }

        return 0;
    }

    int getTypeSize(int nType)
    {
        int nNumTypes = sizeof(SupportedTypes) / sizeof(SupportedTypes[0]);

        for (int i = 0; i < nNumTypes; i++)
        {
            if (SupportedTypes[i].nType == nType)
            {
                return SupportedTypes[i].nSizeInByte;
            }
        }

        return 0;
    }

    bool getTypeName(int nType, std::string &ostrName)
    {
        int nNumTypes = sizeof(SupportedTypes) / sizeof(SupportedTypes[0]);

        for (int i = 0; i < nNumTypes; i++)
        {
            if (SupportedTypes[i].nType == nType)
            {
                ostrName = SupportedTypes[i].strName;

                return true;
            }
        }
        ostrName.clear();
        return false;
    }


    int getType(int idx)
    {
        if (idx < getNumSupportedTypes())
            return SupportedTypes[idx].nType;

        return 0;
    }


    int getNumSupportedTypes()
    {
        return sizeof(SupportedTypes) / sizeof(SupportedTypes[0]);
    }


    int getAlignment(int nWidth, int nExtFormat, int nType)
    {
        int nAlignment = 1;
        int nRowWidth = getExternalFormatSize(nExtFormat, nType) * nWidth;

        if (nRowWidth % 8 == 0)
        {
            nAlignment = 8;
        }
        else if (nRowWidth % 4 == 0)
        {
            nAlignment = 4;
        }
        else if (nRowWidth % 2 == 0)
        {
            nAlignment = 2;
        }
        
        return nAlignment;
    }
}