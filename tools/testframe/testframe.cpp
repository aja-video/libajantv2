/* SPDX-License-Identifier: MIT */
/**
	@file		crossplatform/testframe/testframe.cpp
	@brief		Implements 'testframe' command-line tool, for testing NTV2 DMA functionality.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.
**/

#include <stdio.h>
#include <iostream>
#include <string>
#include <signal.h>
#if defined(AJALinux)
#include <stdlib.h>  // For rand
#endif

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"


using namespace std;

#ifdef MSWindows
#pragma warning(disable : 4996)
#endif

#ifdef AJALinux
#include "ntv2linuxpublicinterface.h"
#endif

#define MY_PAGE_SIZE 4096
#define MY_PAGE_MASK 0xfffffffffffff000ULL

static int s_iBoard = 0;
static NTV2DMAEngine s_eWriteEngine = NTV2_DMA1;
static NTV2DMAEngine s_eReadEngine = NTV2_DMA1;
static ULWord s_ulData0 = 0;
static ULWord s_ulData1 = 0;
static ULWord s_ulDataCount = 0;
static ULWord s_ulDataWidth = 0;
static int s_iIndexFirst = 0;
static int s_iIndexLast = 1000;
static bool s_bRandomData = true;
static char s_BinFileName[MAX_PATH];
static char s_LogFileName[MAX_PATH];
static bool s_bBin = false;
static bool s_bLog = false;
static bool s_bVerbose = false;
static bool s_bRealVerbose = false;
static int s_iVerbose = 20;
static bool s_bQuiet = false;
static int s_iTestCount = 0;
static bool s_bCompare = false;
static ULWord s_ulSegBytes;
static ULWord s_ulSegCount;
static ULWord s_ulSegHostPitch;
static ULWord s_ulSegCardPitch;
static ULWord s_ulSegCardOffset;
static bool s_bSegment = false;
static int s_iSystemSize = 0x800000;
static int s_iFrameOffset = 0;
static int s_iSystemOffset = 0;
static int s_bRandomAddress = 0;
static ULWord s_uRandomMask = 0xfffffffc;
static bool s_bDmaSerialize = true;
static bool s_bFastCopy = false;
static bool s_bCountData = false;
static bool s_bUserData = false;
static ULWord s_ulUserCount = 0;
static bool s_bAlternateData = false;

//static ULWord s_ulTrigger = 0x55555555;


static ULWord s_ulRandomW;
static ULWord s_ulRandomX;
static ULWord s_ulRandomY;
static ULWord s_ulRandomZ;


void Random(ULWord* pNumber)
{
	ULWord ulRandomT = (s_ulRandomX^(s_ulRandomX<<11)); 
	s_ulRandomX = s_ulRandomY;
	s_ulRandomY = s_ulRandomZ; 
	s_ulRandomZ = s_ulRandomW;
	s_ulRandomW = (s_ulRandomW^(s_ulRandomW>>19))^(ulRandomT^(ulRandomT>>8));

	if(pNumber != NULL)
	{
		*pNumber = s_ulRandomW;
	}

	return;
}

int CharToInt(char c)
{
	int i = (-1);

	switch(c)
	{
	case '0':
		i = 0;
		break;
	case '1':
		i = 1;
		break;
	case '2':
		i = 2;
		break;
	case '3':
		i = 3;
		break;
	case '4':
		i = 4;
		break;
	case '5':
		i = 5;
		break;
	case '6':
		i = 6;
		break;
	case '7':
		i = 7;
		break;
	case '8':
		i = 8;
		break;
	case '9':
		i = 9;
		break;
	default:
		break;
	}

	return i;
}


void DoCopy(void* pDst, void* pSrc, int iSize)
{
	ULWord* pRead = (ULWord*)pSrc;
	ULWord* pWrite = (ULWord*)pDst;
	int iCount = iSize/4;
	int i;

	if(s_bFastCopy)
	{
		memcpy(pDst, pSrc, iSize);
	}
	else
	{
		for(i = 0; i < iCount; i++)
		{
			*pWrite++ = *pRead++;
		}
	}
}


void DoSet(void* pDst, UByte uByte, int iSize)
{
	ULWord* pWrite = (ULWord*)pDst;
	int iCount = iSize/4;
	ULWord uData = (uByte << 24) | (uByte << 16) | (uByte << 8) | uByte;
	int i;

	if(s_bFastCopy)
	{
		memset(pDst, uByte, iSize);
	}
	else
	{
		for(i = 0; i < iCount; i++)
		{
			*pWrite++ = uData;
		}
	}
}

void SignalHandler(int signal)
{
	(void) signal;
	s_iTestCount = 1;
}

int main(int argc, char* argv[])
{
	int iData0 = 0;
	int iData1 = 0;
	int iData2 = 0;
	int iData3 = 0;
	int iData4 = 0;
	ULWord ulDataCount = 0;
	ULWord ulData0 = 0;
	ULWord ulData1 = 0;
	bool bParseError = false;
	const int iStrSize = 50;
	FILE* pLogFile = NULL;
	FILE* pBinFile = NULL;
	bool bSuccess = true;

	try
	{
		signal(SIGINT, SignalHandler);

		s_ulRandomW = rand();
		s_ulRandomX = rand();
		s_ulRandomY = rand();
		s_ulRandomZ = rand();
		strcpy(s_BinFileName, "testframe.bin");
		strcpy(s_LogFileName, "testframe.log");

		int iArg = 1;
		while(iArg < argc)
		{
			if(argv[iArg][0] == '-')
			{
				switch(argv[iArg][1])
				{
				case 'a':
				case 'A':
					{
						s_bAlternateData = true;
						s_bRandomData = false;
						break;
					}
				case 'b':
				case 'B':
					{
						ulData0 = 0;
						if(sscanf(&argv[iArg][2], "%i", &iData0) == 1)
						{
							s_iBoard = iData0;
						}
						else
						{
							printf("error: missing board number\n");
							bParseError = true;
						}
						break;
					}
				case 'e':
				case 'E':
					{
						if(!bParseError)
						{
							switch(argv[iArg][2])
							{
							case '0':
								s_eWriteEngine = NTV2_PIO;
								break;
							case '1':
								s_eWriteEngine = NTV2_DMA1;
								break;
							case '2':
								s_eWriteEngine = NTV2_DMA2;
								break;
							case '3':
								s_eWriteEngine = NTV2_DMA3;
								break;
							case '4':
								s_eWriteEngine = NTV2_DMA4;
								break;
							case '#':
								s_bCompare = true;
								AJA_FALL_THRU;
							case '*':
								s_eWriteEngine = (NTV2DMAEngine)999;
								break;
							case '\0':
								{
									printf("error: missing engine number\n");
									bParseError = true;
									break;
								}
							default:
								{
									printf("error: bad write engine %c\n", argv[iArg][2]);
									bParseError = true;
								}
							}
						}
						if(!bParseError)
						{
							switch(argv[iArg][3])
							{
							case '0':
								s_eReadEngine = NTV2_PIO;
								break;
							case '1':
								s_eReadEngine = NTV2_DMA1;
								break;
							case '2':
								s_eReadEngine = NTV2_DMA2;
								break;
							case '3':
								s_eReadEngine = NTV2_DMA3;
								break;
							case '4':
								s_eReadEngine = NTV2_DMA4;
								break;
							case '#':
								s_bCompare = true;
								AJA_FALL_THRU;
							case '*':
								s_eReadEngine = (NTV2DMAEngine)999;
								break;
							case '\0':
								{
									s_eReadEngine = s_eWriteEngine;
									break;
								}
							default:
								{
									printf("error: bad read engine %c\n", argv[iArg][3]);
									bParseError = true;
								}
							}
						}
						break;
					}
				case 'c':
				case 'C':
					{
						s_bCountData = true;
						s_bRandomData = false;
						break;
					}
				case 'd':
				case 'D':
					{
						ulData0 = 0;
						ulData1 = 0;
						iData0 = 0;
						ulDataCount = sscanf(&argv[iArg][2], "%x/%x/%i", &ulData0, &ulData1, &iData0);
						if(ulDataCount != 0)
						{
							s_ulDataCount = ulDataCount;
							s_ulData0 = ulData0;
							s_ulData1 = ulData1;
							s_ulDataWidth = (ULWord)iData0;
							s_bUserData = true;
							s_bRandomData = false;
						}
						else
						{
							printf("error: missing data\n");
							bParseError = true;
						}
						break;
					}
				case 'f':
				case 'F':
					{
						iData0 = 0;
						iData1 = 0;
						ulDataCount = sscanf(&argv[iArg][2], "%i/%i", &iData0, &iData1);
						if(ulDataCount == 2)
						{
							s_iIndexFirst = iData0;
							s_iIndexLast = iData1;
						}
						else
						{
							printf("error: missing frame index\n");
							bParseError = true;
						}
						break;
					}
				case 'g':
				case 'G':
					{
						iData0 = 0;
						if(sscanf(&argv[iArg][2], "%i", &iData0) == 1)
						{
							s_iSystemSize = iData0;
						}
						else
						{
							printf("error: missing frame size\n");
							bParseError = true;
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
							strcpy(s_BinFileName, &argv[iArg][2]);
						}
						s_bBin = true;
						s_bRandomData = false;
						break;
					}
				case 'o':
				case 'O':
					{
						iData0 = 0;
						if(sscanf(&argv[iArg][2], "%i", &iData0) == 1)
						{
							s_iFrameOffset = iData0;
						}
						else
						{
							printf("error: missing frame offset\n");
							bParseError = true;
						}
						break;
					}
				case 'p':
				case 'P':
					{
						iData0 = 0;
						if(sscanf(&argv[iArg][2], "%i", &iData0) == 1)
						{
							s_iSystemOffset = iData0;
						}
						else
						{
							printf("error: missing system offset\n");
							bParseError = true;
						}
						break;
					}
				case 'r':
				case 'R':
					{
						s_bRandomAddress = true;
						iData0 = 0;
						if(sscanf(&argv[iArg][2], "%i", &iData0) == 1)
						{
							iData1 = 0;
							s_uRandomMask = 0xffffffff;
							while((iData0 & 0x1) == 0)
							{
								iData0 = iData0 >> 1;
								s_uRandomMask = s_uRandomMask >> 1;
								iData1++;
							}
							while(iData1 > 0)
							{
								s_uRandomMask = s_uRandomMask << 1;
								iData1--;
							}
						}
						break;
					}
				case 's':
				case 'S':
					{
						iData0 = 0;
						iData1 = 0;
						iData2 = 0;
						iData3 = 0;
						iData4 = 0;
						ulDataCount = sscanf(&argv[iArg][2], "%i/%i/%i/%i/%i", &iData0, &iData1, &iData2, &iData3, &iData4);
						if((ulDataCount == 0) || (ulDataCount > 5))
						{
							s_ulSegBytes = 64;
							s_ulSegCount = 4;
							s_ulSegHostPitch = 128;
							s_ulSegCardPitch = 256;
							s_ulSegCardOffset = 16;
							s_bSegment = true;
							s_bRandomData = false;
						}
						else if(ulDataCount == 5)
						{
							s_ulSegBytes = (ULWord)iData0 & 0xfffffffc;
							s_ulSegCount = (ULWord)iData1;
							s_ulSegHostPitch = (ULWord)iData2 & 0xfffffffc;
							s_ulSegCardPitch = (ULWord)iData3 & 0xfffffffc;
							s_ulSegCardOffset = (ULWord)iData4 & 0xfffffffc;
							s_bSegment = true;
							s_bRandomData = false;
						}
						else
						{
							printf("error: bad segment data\n");
							bParseError = true;
						}
						break;
					}
				case 't':
				case 'T':
					{
						iData0 = 0;
						if(sscanf(&argv[iArg][2], "%i", &iData0) == 1)
						{
							s_iTestCount = iData0;
						}
						else
						{
							printf("error: missing test count\n");
							bParseError = true;
						}
						break;
					}
				case 'u':
				case 'U':
					{
						s_bFastCopy = true;
						break;
					}
				case 'v':
				case 'V':
					{
						s_bVerbose = true;
						iData0 = 0;
						if(sscanf(&argv[iArg][2], "%i", &iData0) == 1)
						{
							s_iVerbose = iData0;
						}
						break;
					}
				case 'x':
				case 'X':
					{
						s_bVerbose = true;
						s_bRealVerbose = true;
						break;
					}
				case 'z':
				case 'Z':
					{
						s_bDmaSerialize = false;
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
					printf("usage:  testframe [switches]\n");
					printf("  -a             alternating buffer test\n");
					printf("  -bN            N = board number\n");
					printf("  -eW[R]         W = write engine number  R = read engine number (0 = pio, * = none)\n");
					printf("  -c             counting number test values\n");
					printf("  -dX[/Y][/W]    X = test value (hex)  Y = alternate value (hex)  W = width (dwords)\n");
					printf("  -fF/L          F = first frame index L = last frame index\n");
					printf("  -gS            S = frame size\n");
					printf("  -h -?          help\n");
					printf("  -l[NAME]       NAME = log file name (testframe.log)\n");
					printf("  -n[NAME]       NAME = binary file name (testframe.bin)\n");
					printf("  -oO            O = frame buffer offset\n");
					printf("  -pO            O = system buffer offset\n");
					printf("  -q             quiet output (batch)\n");
					printf("  -r[M]          random offset/size M = mask (0xfffffffc)\n");
					printf("  -s[S/N/H/C/X]  S = size  N = count  H = host pitch  C = card pitch  X = card offset\n");
					printf("  -tN            N = number of test frames\n");
					printf("  -u             use fast pio\n");
					printf("  -vN            verbose output  N = error count\n");
					printf("  -x             real verbose output (output all)\n");
					printf("  -z             disable dma serialize\n");
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

		if(s_bRandomData || s_bSegment)
		{
			s_ulDataWidth = 1;
		}

		if(s_ulDataWidth == 0)
		{
			s_ulDataWidth = 1;
		}

		char sCount[iStrSize];
		if(s_iTestCount == 0)
		{
			strcpy(sCount, "infinite");
		}
		else
		{
			sprintf(sCount, "%d", s_iTestCount);
		}

		if(s_bLog)
		{
			pLogFile = fopen(s_LogFileName, "a");
			if(pLogFile == NULL)
			{
				printf("error: can not open log file %s\n", s_LogFileName);
				throw 0;
			}
		}

		if(!s_bQuiet)
		{
			printf("\ntest board %d  write %d  read %d  width %d  count %s\n\n", 
				s_iBoard, s_eWriteEngine, s_eReadEngine, s_ulDataWidth, sCount);
		}

		if(pLogFile != NULL)
		{
			fprintf(pLogFile, "\ntest board %d  dma write %d  dma read %d  count %s\n\n", 
				s_iBoard, s_eWriteEngine, s_eReadEngine, sCount);
		}

		NTV2DeviceInfo boardInfo;
		CNTV2DeviceScanner ntv2BoardScan;
		if (ntv2BoardScan.GetNumDevices() <= (ULWord)s_iBoard)
		{
			printf("error: opening device %d failed\n", s_iBoard);
			throw 0;
		}
		boardInfo = ntv2BoardScan.GetDeviceInfoList()[s_iBoard];

		CNTV2Card avCard;
		avCard.Open(boardInfo.deviceIndex);

		// find the board
		if(!avCard.IsOpen())
		{
			printf("error: opening device %d failed\n", s_iBoard);
			throw 0;
		}

#if 0
		NTV2DeviceID eBoardID = avCard.GetDeviceID();

		NTV2FrameBufferFormat frameBufferFormat;
		avCard.GetFrameBufferFormat(NTV2_CHANNEL1, &frameBufferFormat);

		NTV2FrameGeometry frameGeomety;
		avCard.GetFrameGeometry(&frameGeomety);

		int iFrameSize = ::NTV2DeviceGetFrameBufferSize(eBoardID, frameGeomety, frameBufferFormat);
		UWord numberOfAudioStreams = ::NTV2DeviceGetNumAudioSystems(eBoardID);
		int iFrameCount = ::NTV2DeviceGetNumberFrameBuffers(eBoardID, frameGeomety, frameBufferFormat) - numberOfAudioStreams;
//		printf("frame size %d  frame buffers %d\n", iFrameSize, iFrameCount);

		if((s_iSystemSize == 0) || (s_iSystemSize > iFrameSize))
		{
			s_iSystemSize = iFrameSize;
		}

		if (iFrameSize == 0)
		{
			printf("error: board %d frame size is zero\n", s_iBoard);
			throw 0;
		}
		if (iFrameCount == 0)
		{
			printf("error: board %d frame count is zero\n", s_iBoard);
			throw 0;
		}

		if(s_iIndexFirst > iFrameCount)
		{
			s_iIndexFirst = iFrameCount - 1;
		}
		if(s_iIndexLast > iFrameCount)
		{
			s_iIndexLast = iFrameCount - 1;
		}
		if(s_iIndexFirst < 0)
		{
			s_iIndexFirst = 0;
		}
		if(s_iIndexLast < 0)
		{
			s_iIndexLast  = 0;
		}
#endif

		NTV2DeviceID eBoardID = avCard.GetDeviceID();

		for(uint32_t i = 0; i < NTV2DeviceGetNumVideoChannels(eBoardID); i++)
		{
			avCard.SetMode((NTV2Channel)i, NTV2_MODE_DISPLAY);
		}
		for(uint32_t i = 0; i < NTV2DeviceGetNumAudioSystems(eBoardID); i++)
		{
			avCard.StopAudioInput((NTV2AudioSystem)i);
		}
		for(uint32_t i = 0; i < NTV2DeviceGetNumVideoInputs(eBoardID); i++)
		{
			avCard.AncExtractSetEnable(i, false);
		}
		int iFrameSize = 0x1000000;
		if (NTV2DeviceCanDo4KVideo(eBoardID))
			iFrameSize = 0x4000000;
		if (NTV2DeviceCanDo8KVideo(eBoardID))
			iFrameSize = 0x10000000;

		ULWord* pSrcBufferAlloc = (ULWord*)new char[iFrameSize + MY_PAGE_SIZE];
		ULWord*	pDstBufferAlloc = (ULWord*)new char[iFrameSize + MY_PAGE_SIZE];
		ULWord*	pZeroBufferAlloc = (ULWord*)new char[iFrameSize + MY_PAGE_SIZE];

		ULWord* pSrcBuffer = (ULWord*)((((ULWord64)pSrcBufferAlloc) + MY_PAGE_SIZE) & MY_PAGE_MASK);
		ULWord* pDstBuffer = (ULWord*)((((ULWord64)pDstBufferAlloc) + MY_PAGE_SIZE) & MY_PAGE_MASK);
		ULWord* pZeroBuffer = (ULWord*)((((ULWord64)pZeroBufferAlloc) + MY_PAGE_SIZE) & MY_PAGE_MASK);

		memset(pZeroBuffer, 0x22, iFrameSize);

		int iFrame = 0;
		int iSrc = 0;
		int iDst = 0;
		int iIndex = 0;
		ULWord ulWidth = 0;
		bool bData0 = true;

		if(s_bBin)
		{
			pBinFile = fopen(s_BinFileName, "rb");
			if(pBinFile == NULL)
			{
				printf("error: can not open bin file %s\n", s_BinFileName);
				throw 0;
			}

			int iSize = (int)fread(pSrcBuffer, 4, s_iSystemSize/4, pBinFile);
			if(ferror(pBinFile))
			{
				printf("error: can not read bin file %s\n", s_BinFileName);
				throw 0;
			}

			fclose(pBinFile);

			iSize *= 4;
			if(iSize < s_iSystemSize)
			{
				s_iSystemSize = iSize;
			}
		}
		else if(s_bSegment)
		{
			DoSet(pSrcBuffer, 0, iFrameSize);

			ULWord ulSeg = 0;
			ULWord ulDat = 0;
			for(ulSeg = 0; ulSeg < s_ulSegCount; ulSeg++)
			{
				for(ulDat = 0; ulDat < s_ulSegBytes/4; ulDat++)
				{
					int iSrc = ulSeg * s_ulSegHostPitch/4 + ulDat;
				    if(iSrc < s_iSystemSize/4)
					{
						pSrcBuffer[iSrc] = 0xffffffff;
					}
				}
			}
		}
		else if(s_bUserData)
		{
			for(iSrc = 0; iSrc < iFrameSize/4; iSrc++)
			{
				if(s_ulDataCount == 1)
				{
					pSrcBuffer[iSrc] = s_ulData0;
				}
				else
				{
					if(bData0)
					{
						pSrcBuffer[iSrc] = s_ulData0;
					}
					else
					{
						pSrcBuffer[iSrc] = s_ulData1;
					}
					ulWidth++;
					if(ulWidth >= s_ulDataWidth)
					{
						bData0 = !bData0;
						ulWidth = 0;
					}
				}
			}
		}
		else if(s_bAlternateData)
		{
			for(iSrc = 0; iSrc < iFrameSize/4; iSrc++)
			{
				pSrcBuffer[iSrc] = iSrc;
				pZeroBuffer[iSrc] = iSrc | 0xf0000000;
			}
		}

		if((s_iSystemOffset + s_iSystemSize) > iFrameSize)
		{
			s_iSystemSize = iFrameSize - s_iSystemOffset;
		}
		if((s_iFrameOffset + s_iSystemSize) > iFrameSize)
		{
			s_iSystemSize = iFrameSize - s_iFrameOffset;
		}
		if(s_iSystemSize == 0)
		{
			s_iSystemSize = iFrameSize;
		}

		unsigned char* pFrameBuffer = 0;
#if 0
		avCard.GetBaseAddress((ULWord**)&pFrameBuffer);
		if(pFrameBuffer == NULL)
		{
			printf("error: can not get frame buffer address\n");
		}
#endif
		// disable dma serialization
#ifdef MSWindows
		ULWord ulDmaSerialize = 0;
		if(!s_bDmaSerialize)
		{
			avCard.ReadRegister(kVRegDmaSerialize, ulDmaSerialize);
			avCard.WriteRegister(kVRegDmaSerialize, 0);
		}
#endif
		int iTest = 0;
		int iError = 0;
		bool bTestFormat = true;
		ULWord* pTstBuffer = NULL;
		while((s_iTestCount == 0) || (iTest < s_iTestCount))
		{
			pTstBuffer = pSrcBuffer;
			if(s_bAlternateData && ((iTest % 2) != 0))
			{
				pTstBuffer = pZeroBuffer;
			}

			if(s_bRandomData)
			{
				for(iSrc = 0; iSrc < iFrameSize/4; iSrc++)
				{
					Random(&pTstBuffer[iSrc]);
				}
			}

			if(s_bRandomAddress)
			{
				int iMinSize = (~s_uRandomMask) + 1;

				ULWord uRandom;
				Random(&uRandom);
				s_iSystemOffset = (uRandom % (iFrameSize - iMinSize)) & s_uRandomMask;
				Random(&uRandom);
				s_iFrameOffset = (uRandom % (iFrameSize - iMinSize)) & s_uRandomMask;

				int iSystemSize = iFrameSize;
				if((s_iSystemOffset + iSystemSize) > iFrameSize)
				{
					iSystemSize = iFrameSize - s_iSystemOffset;
				}
				if((s_iFrameOffset + iSystemSize) > iFrameSize)
				{
					iSystemSize = iFrameSize - s_iFrameOffset;
				}

				if(iSystemSize < iMinSize)
				{
					printf("error: can not generate random offset/size\n");
					throw 0;
				}

				Random(&uRandom);
				s_iSystemSize = (uRandom % iSystemSize) & s_uRandomMask;
				if(s_iSystemSize < iMinSize)
				{
					s_iSystemSize = iMinSize;
				}
			}

			for(iFrame = s_iIndexFirst; iFrame <= s_iIndexLast; iFrame++)
			{
				if(s_bCountData)
				{
					for(iSrc = 0; iSrc < iFrameSize/4; iSrc++)
					{
						pTstBuffer[iSrc] = s_ulUserCount++;
					}
				}
				if(s_eWriteEngine == NTV2_PIO)
				{
					if(pFrameBuffer != NULL)
					{
						DoCopy(pFrameBuffer + iFrame*iFrameSize + s_iFrameOffset, pTstBuffer + s_iSystemOffset/4, s_iSystemSize);
					}
				}
				else if(s_eWriteEngine <= NTV2_DMA4)
				{
					if(s_bSegment)
					{	//	DMAWrite
						avCard.DmaTransfer (s_eWriteEngine, false, iFrame, pZeroBuffer, ULWord (0), iFrameSize, true);
						//	DMAWriteSegments
						avCard.DmaTransfer (s_eWriteEngine, false, iFrame, pTstBuffer, s_ulSegCardOffset, s_ulSegBytes,
											s_ulSegCount, s_ulSegHostPitch, s_ulSegCardPitch, true);
					}
					else
					{	//	DMAWrite
						avCard.DmaTransfer (s_eWriteEngine, false, iFrame, pTstBuffer + s_iSystemOffset/4, s_iFrameOffset, s_iSystemSize);
					}
				}

				memset(pDstBuffer, 0xaa, iFrameSize);
				if(s_eReadEngine == NTV2_PIO)
				{
					if(pFrameBuffer != NULL)
					{
						DoCopy(pDstBuffer + s_iSystemOffset/4, pFrameBuffer + iFrame*iFrameSize + s_iFrameOffset, s_iSystemSize);
					}
				}
				else if(s_eReadEngine <= NTV2_DMA4)
				{
					if(s_bSegment)
					{	//	DMAReadSegments
						avCard.DmaTransfer (s_eReadEngine, true, iFrame, pDstBuffer, s_ulSegCardOffset, s_ulSegBytes,
											s_ulSegCount, s_ulSegHostPitch, s_ulSegCardPitch, true);
					}
					else
					{	//	DMARead
						avCard.DmaTransfer (s_eReadEngine, true, iFrame, pDstBuffer + s_iSystemOffset/4, s_iFrameOffset, s_iSystemSize, true);
					}
				}
				
				int iECount = 0;
				int iVCount = 0;
				if(((s_eWriteEngine <= NTV2_DMA4) && (s_eReadEngine <= NTV2_DMA4)) || s_bCompare)
				{
					for(iIndex = 0; iIndex < s_iSystemSize/4; iIndex++)
					{
						iDst = s_iSystemOffset/4 + iIndex;
						if(pTstBuffer[iDst] != pDstBuffer[iDst])
						{
							iECount++;
							ULWord ulDstData = 0;
							bSuccess = false;
							if((iVCount < s_iVerbose) && (s_bVerbose || (pLogFile != NULL)))
							{
								if(pFrameBuffer != NULL)
								{
//									DoCopy(&ulDstData, pFrameBuffer + iFrame*iFrameSize + s_iFrameOffset + iIndex*4, 4);
//									DoCopy(pFrameBuffer, &s_ulTrigger, 4);
								}
								if(bTestFormat)
								{
									if(!s_bQuiet && s_bVerbose)
									{
										printf("\n");
										printf("frm %9d  fb %2d  osf %08x %08x  size %08x\n",
											iTest, iFrame, s_iSystemOffset, s_iFrameOffset, s_iSystemSize);
									}
									if(pLogFile != NULL)
									{
										fprintf(pLogFile, "frm %9d  fb %2d  osf %08x %08x  size %08x\n",
											iTest, iFrame, s_iSystemOffset, s_iFrameOffset, s_iSystemSize);
									}
								}
								if(!s_bQuiet && s_bVerbose)
								{
									printf("frm %9d  fb %2d  osf %08x %08x  sdxp %08x %08x %08x %08x\n", 
										iTest, iFrame, s_iSystemOffset + iIndex*4, s_iFrameOffset + iIndex*4, 
										pTstBuffer[iDst], pDstBuffer[iDst], 
										pTstBuffer[iDst]^pDstBuffer[iDst], ulDstData);
								}
								if(pLogFile != NULL)
								{
									fprintf(pLogFile, "frm %9d  fb %2d  osf %08x %08x  src %08x  dst %08x  xor %08x  pio %08x\n", 
										iTest, iFrame, s_iSystemOffset + iIndex*4, s_iFrameOffset + iIndex*4, 
										pTstBuffer[iDst], pDstBuffer[iDst], 
										pTstBuffer[iDst]^pDstBuffer[iDst], ulDstData);
								}

								bTestFormat = false;
								if(s_iVerbose > 0)
								{
									iVCount++;
								}
							}
						}
						else
						{
							if(!s_bQuiet && s_bRealVerbose)
							{
								printf("frm %9d  fb %2d  osf %08x %08x  sdxp %08x %08x %08x\n", 
									iTest, iFrame, s_iSystemOffset + iIndex*4, s_iFrameOffset + iIndex*4, 
									pTstBuffer[iDst], pDstBuffer[iDst], 
									pTstBuffer[iDst]^pDstBuffer[iDst]);
							}
						}
					}
				}

				if(iECount > 0)
				{
					iError++;
					if(!s_bQuiet && s_bVerbose)
					{
						printf("frm %9d  fb %2d  diffs %9d\n", iTest, iFrame, iECount); 
					}
					if(pLogFile != NULL)
					{
						fprintf(pLogFile, "frm %9d  fb %2d  diffs %9d\n", iTest, iFrame, iECount); 
						fflush(pLogFile);
					}
				}

				iTest++;

				if(!s_bQuiet)
				{
					printf("frames/errors %d/%d\r", iTest, iError);
					fflush(stdout);
					bTestFormat = true;
				}

				if((s_iTestCount != 0) && (iTest >= s_iTestCount))
				{
					break;
				}
			}
		}
#ifdef MSWindows
		avCard.WriteRegister(kVRegDmaSerialize, 1); //ulDmaSerialize);
#endif
		if(!s_bQuiet && bTestFormat)
		{
			printf("\n");
		}

		if(pLogFile != NULL)
		{
			fprintf(pLogFile, "\ntested %d frames\n", iTest);
			fclose(pLogFile);
			pLogFile = NULL;
		}

		if(pSrcBufferAlloc != NULL)
		{
			delete [] pSrcBufferAlloc;
		}
		if(pDstBufferAlloc != NULL)
		{
			delete [] pDstBufferAlloc;
		}
		if(pZeroBufferAlloc != NULL)
		{
			delete [] pZeroBufferAlloc;
		}

		if (!bSuccess)
		{
			return -1;
		}

		return 0;
	}
	catch(...)
	{
	}

	return -1;
}
