/* SPDX-License-Identifier: MIT */
/**
	@file		crossplatform/regio/regio.cpp
	@brief		Implements 'regio' command.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include <cinttypes>
#include <stdio.h>
#include <iostream>
#include <string>
#include <signal.h>
#include <list>

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "i2c.h"
#include "ajabase/system/debug.h"
#if !defined(AJA_LINUX) && !defined(AJALinux)
#include "ajabase/system/systemtime.h"
#endif


using namespace std;

#ifdef MSWindows
#pragma warning(disable : 4996)
#endif

#ifdef AJALinux
#include "ntv2linuxpublicinterface.h"
#endif

#define MY_PAGE_SIZE 4096
#define MY_PAGE_MASK 0xfffffffffffff000ULL

static string s_devSpec("0");
static char s_LogFileName[MAX_PATH];
static bool s_bLog = false;
static bool s_bVerbose = false;
static bool s_bQuiet = false;
static bool s_bQuit = false;

struct SRegIO
{
	bool bReg;
    bool bVData;
	bool bI2C;
	bool bWrite;
	union
	{
		struct  
		{
			ULWord ulReg;
			ULWord ulValue;
			ULWord ulMask;
		} Reg;
		struct
		{
			ULWord ulOffset;
			ULWord64 uqData;
			ULWord ulSize;
		} Frame;
	} IO;
};

typedef std::list<SRegIO> RegIOList;

void SignalHandler(int signal)
{
	(void) signal;
	s_bQuit = true;
}

int main(int argc, char* argv[])
{
	int iData0 = 0;
	int iData1 = 0;
	int iData2 = 0;
    ULWord ilDataCount = 0;
	bool bParseError = false;
	FILE* pLogFile = AJA_NULL;

	RegIOList RegList;

	try
	{
		signal(SIGINT, SignalHandler);

		strcpy(s_LogFileName, "regio.log");

		int iArg = 1;
		while(iArg < argc)
		{
			if(argv[iArg][0] == '-')
			{
				switch(argv[iArg][1])
				{
				case 'b':
				case 'B':
				case 'd':
				case 'D':
					{
						s_devSpec = argv[iArg];
						s_devSpec.erase(0,2);	//	Remove '-d'
						if(s_devSpec.empty())
						{
							cerr << "## ERROR:  Missing device specification" << endl;
							bParseError = true;
						}
						break;
					}
				case 'i':
				case 'I':
					{
                        ilDataCount = sscanf(&argv[iArg][2], "%i/%i", &iData0, &iData1);
                        if((ilDataCount == 0) || (ilDataCount > 3))
						{
							printf("error: missing register number\n");
							bParseError = true;
						}
                        else if(ilDataCount == 1)
						{
							printf("error: missing register value\n");
							bParseError = true;
						}
						else
						{
							SRegIO RegData;
							RegData.bReg = false;
                            RegData.bVData = false;
							RegData.bI2C = true;
							RegData.bWrite = true;
							RegData.IO.Reg.ulReg = iData0;
							RegData.IO.Reg.ulValue = iData1;
							RegData.IO.Reg.ulMask = 0xffffffff;
							RegList.push_back(RegData);
						}
						break;
					}
				case 'r':
				case 'R':
					{
                        ilDataCount = sscanf(&argv[iArg][2], "%i/%i", &iData0, &iData1);
                        if((ilDataCount == 0) || (ilDataCount > 2))
						{
							printf("error: missing register number\n");
							bParseError = true;
						}
						else
						{
							SRegIO RegData;
							RegData.bReg = true;
                            RegData.bVData = false;
							RegData.bI2C = false;
							RegData.bWrite = false;
							RegData.IO.Reg.ulReg = iData0;
							RegData.IO.Reg.ulValue = 0;
							RegData.IO.Reg.ulMask = 0xffffffff;
                            if(ilDataCount == 2)
							{
								RegData.IO.Reg.ulMask = iData1;
							}
							RegList.push_back(RegData);
						}
						break;
					}
				case 'w':
				case 'W':
					{
                        ilDataCount = sscanf(&argv[iArg][2], "%i/%i/%i", &iData0, &iData1, &iData2);
                        if((ilDataCount == 0) || (ilDataCount > 3))
						{
							printf("error: missing register number\n");
							bParseError = true;
						}
                        else if(ilDataCount == 1)
						{
							printf("error: missing register value\n");
							bParseError = true;
						}
						else
						{
							SRegIO RegData;
							RegData.bReg = true;
                            RegData.bVData = false;
							RegData.bI2C = false;
							RegData.bWrite = true;
							RegData.IO.Reg.ulReg = iData0;
							RegData.IO.Reg.ulValue = iData1;
							RegData.IO.Reg.ulMask = 0xffffffff;
                            if(ilDataCount == 3)
							{
								RegData.IO.Reg.ulMask = iData2;
							}
							RegList.push_back(RegData);
						}
						break;
					}
				case 'l':
				case 'L':
					{
						if(argv[iArg][2] != '\0')
                        {
							strcpy(s_LogFileName, &argv[iArg][2]);
						}
						s_bLog = true;
						break;
					}
				case 'n':
				case 'N':
					{
						if(argv[iArg][2] != '\0')
						{
							cerr << "## ERROR:  '-n' option is obsolete:  use '-d' option with URL to specify remote devices" << endl;
							bParseError = true;
						}
						break;
					}
                case 't':
                case 'T':
                    {
                        ilDataCount = sscanf(&argv[iArg][2], "%i/%i", &iData0, &iData1);
                        if(ilDataCount == 0)
                        {
                            printf("error: missing tag\n");
                            bParseError = true;
                        }
                        else if(ilDataCount == 1)
                        {
                            printf("error: missing data\n");
                            bParseError = true;
                        }
                        else if(ilDataCount > 2)
                        {
                            printf("error: too many params\n");
                            bParseError = true;
                        }
                        else
                        {
                            SRegIO RegData;
                            RegData.bReg = false;
                            RegData.bVData = true;
                            RegData.bI2C = false;
                            RegData.bWrite = true;
                            RegData.IO.Reg.ulReg = iData0;
                            RegData.IO.Reg.ulValue = iData1;
                            RegData.IO.Reg.ulMask = 0xffffffff;
                            RegList.push_back(RegData);
                        }
                        break;
                    }
                case 'u':
                case 'U':
                    {
                        ilDataCount = sscanf(&argv[iArg][2], "%i", &iData0);
                        if(ilDataCount == 0)
                        {
                            printf("error: missing tag\n");
                            bParseError = true;
                        }
                        else if(ilDataCount > 1)
                        {
                            printf("error: too many params\n");
                            bParseError = true;
                        }
                        else
                        {
                            SRegIO RegData;
                            RegData.bReg = false;
                            RegData.bVData = true;
                            RegData.bI2C = false;
                            RegData.bWrite = false;
                            RegData.IO.Reg.ulReg = iData0;
                            RegData.IO.Reg.ulValue = 0;
                            RegData.IO.Reg.ulMask = 0xffffffff;
                            RegList.push_back(RegData);
                        }
                        break;
                    }
                case 'v':
				case 'V':
					{
						s_bVerbose = true;
						break;
					}
				case 'q':
				case 'Q':
					{
						s_bQuiet = true;
						break;
					}
				case 'h':
				case 'H':
				case '?':
					printf("usage:  regio [switches]\n");
					printf("  -d[devSpec]    device or URL (-d? lists local devices)\n");
					printf("  -rN[/M]        read register N  mask M\n");
					printf("  -wN/D[/M]      write register N  value D  mask M\n");
                    printf("  -uN            read virtual data tag N\n");
                    printf("  -tN/D          write virtual data tag N  value D\n");
					printf("  -h -?          help\n");
					printf("  -iN[/D]        write i2c register N  value D\n");
					printf("  -l[NAME]       NAME = log file name (testitx.log)\n");
					printf("  -q             quiet output\n");
					printf("  -v             verbose output\n");
				printf("\n");
					return 0;
				default:
					printf("invalid switch %c", argv[iArg][1]);
					bParseError = true;
					break;
				}
			}
			else
			{
				printf("error: bad parameter %s\n", argv[iArg]);
				bParseError = true;
			}
			iArg++;
		}

		if(bParseError)
		{
			throw 0;
		}

		if(s_bQuit)
		{
			throw 0;
		}

		if(s_bLog)
		{
			pLogFile = fopen(s_LogFileName, "a");
			if (!pLogFile)
			{
				printf("error: can not open log file %s\n", s_LogFileName);
				throw 0;
			}
		}

		if(s_bQuit)
		{
			throw 0;
		}

		AJADebug::Open();
		CNTV2Card avCard;
		CNTV2DeviceScanner::GetFirstDeviceFromArgument (s_devSpec, avCard);

		// find the board
		if (!avCard.IsOpen())
		{
			cerr << "## ERROR: Failed to open '" << s_devSpec << "'" << endl;
			throw 0;
		}

		if(s_bQuit)
		{
			throw 0;
		}

		const char* pError = "";
		const char* pMode = "";

		RegIOList::iterator iterReg;
		for(iterReg = RegList.begin(); iterReg != RegList.end(); iterReg++)
		{
			if(s_bQuit)
			{
				throw 0;
			}

			pError = "";
			SRegIO RegData = *iterReg;

			if(RegData.bReg)
			{
#if !defined(AJA_LINUX) && !defined(AJALinux)
				int64_t before = AJATime::GetSystemMicroseconds();
#else
				int64_t before = 0;
#endif
				if(RegData.bWrite)
				{
					pMode = "write";
					if(!avCard.WriteRegister(RegData.IO.Reg.ulReg, RegData.IO.Reg.ulValue, RegData.IO.Reg.ulMask))
					{
						pError = "  failed";
					}
				}
				else
				{
					pMode = "read ";
					if(!avCard.ReadRegister(RegData.IO.Reg.ulReg, RegData.IO.Reg.ulValue, RegData.IO.Reg.ulMask))
						pError = "  failed";
				}
#if !defined(AJA_LINUX) && !defined(AJALinux)
				int64_t after = AJATime::GetSystemMicroseconds();
#else
				int64_t after = 0;
#endif

				if(s_bQuiet)
				{
					if (!RegData.bWrite)
						// When in read, print ONLY the value read back when in quiet mode
						printf("0x%08x\n", RegData.IO.Reg.ulValue);
				}
				else
					printf("%s reg  board %s  reg %5d  value %08x  mask %08x%s  time(us) %d\n", 
						pMode, s_devSpec.c_str(), RegData.IO.Reg.ulReg, RegData.IO.Reg.ulValue, RegData.IO.Reg.ulMask, pError, (uint32_t)(after-before));
				if(s_bLog)
					fprintf(pLogFile, "%s reg  board %s  reg %5d  value %08x  mask %08x%s\n", 
						pMode, s_devSpec.c_str(), RegData.IO.Reg.ulReg, RegData.IO.Reg.ulValue, RegData.IO.Reg.ulMask, pError);
			}
            else if(RegData.bVData)
            {
#if !defined(AJA_LINUX) && !defined(AJALinux)
                int64_t before = AJATime::GetSystemMicroseconds();
#else
                int64_t before = 0;
#endif

                pError = "";

                if(RegData.bWrite)
                {
                    pMode = "write";
                    if(!avCard.WriteVirtualData(RegData.IO.Reg.ulReg, &RegData.IO.Reg.ulValue, sizeof(ULWord)))
                        pError = "  failed";
                }
                else
                {
                    pMode = "read";
                    if(!avCard.ReadVirtualData(RegData.IO.Reg.ulReg, &RegData.IO.Reg.ulValue, sizeof(ULWord)))
                        pError = "  failed";
                }

#if !defined(AJA_LINUX) && !defined(AJALinux)
                int64_t after = AJATime::GetSystemMicroseconds();
#else
                int64_t after = 0;
#endif

                printf("VData %s Tag %5d  Data %08x%s  time(us) %d\n",
                    pMode, RegData.IO.Reg.ulReg, RegData.IO.Reg.ulValue, pError, (uint32_t)(after-before));

            }
			else if(RegData.bI2C)
			{
				if(RegData.bWrite)
				{
					pMode = "write";
					if(!I2CWriteDataSingle(&avCard, RegData.IO.Reg.ulReg, RegData.IO.Reg.ulValue))
					{
						pError = "  failed";
					}
				}
				else
				{
					pMode = "read ";
					pError = "  failed";
				}

				if(!s_bQuiet)
				{
					printf("%s i2c  board %s  reg %5d  value %08x  mask %08x%s\n", 
						pMode, s_devSpec.c_str(), RegData.IO.Reg.ulReg, RegData.IO.Reg.ulValue, RegData.IO.Reg.ulMask, pError);
				}
				if(s_bLog)
				{
					fprintf(pLogFile, "%s i2c  board %s  reg %5d  value %08x  mask %08x%s\n", 
						pMode, s_devSpec.c_str(), RegData.IO.Reg.ulReg, RegData.IO.Reg.ulValue, RegData.IO.Reg.ulMask, pError);
				}
			}
		}

		if(pLogFile != NULL)
		{
			fclose(pLogFile);
			pLogFile = NULL;
		}

		return 0;
	}
	catch(...)
	{
	}

	return -1;
}
