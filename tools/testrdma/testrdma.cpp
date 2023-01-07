////////////////////////////////////////////////////////
//
// testrdma.cpp 
//
////////////////////////////////////////////////////////

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

#ifdef AJA_RDMA
#include <cuda.h>
#include <cuda_runtime_api.h>
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

static int s_iBoard = 0;
static ULWord s_ulData0 = 0;
static ULWord s_ulData1 = 0;
static ULWord s_ulDataCount = 0;
static ULWord s_ulDataWidth = 0;
static int s_iIndexFirst = 0;
static int s_iIndexLast = 7;
static bool s_bRandomData = true;
static char s_LogFileName[MAX_PATH];
static bool s_bLog = false;
static bool s_bVerbose = false;
static bool s_bRealVerbose = false;
static int s_iVerbose = 20;
static int s_iTestCount = 0;
static int s_iSystemSize = 0x800000;
static bool s_bCountData = false;
static bool s_bUserData = false;
static ULWord s_ulUserCount = 0;
static bool s_bRDMA = true;

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


void SignalHandler(int signal)
{
	(void) signal;
	s_iTestCount = 1;
}

int main(int argc, char* argv[])
{
	int iData0 = 0;
	int iData1 = 0;
	ULWord ulDataCount = 0;
	ULWord ulData0 = 0;
	ULWord ulData1 = 0;
	bool bParseError = false;
	const int iStrSize = 50;
	FILE* pLogFile = NULL;
	bool bSuccess = true;
#ifdef AJA_RDMA	
	ULWord** pCudaBuffer = NULL;
	unsigned int flag = 1;
	cudaError_t ce;
	CUresult cr;
#endif
	
	try
	{
		signal(SIGINT, SignalHandler);

		s_ulRandomW = rand();
		s_ulRandomX = rand();
		s_ulRandomY = rand();
		s_ulRandomZ = rand();
		strcpy(s_LogFileName, "testrdma.log");

		int iArg = 1;
		while(iArg < argc)
		{
			if(argv[iArg][0] == '-')
			{
				switch(argv[iArg][1])
				{
				case 'c':
				case 'C':
					{
						s_bCountData = true;
						s_bRandomData = false;
						s_bUserData = false;
						break;
					}
				case 'd':
				case 'D':
					{
						ulData0 = 0;
						if(sscanf(&argv[iArg][2], "%i", &iData0) == 1)
						{
							s_iBoard = iData0;
						}
						else
						{
							printf("error: missing device number\n");
							bParseError = true;
						}
						break;
					}
				case 'e':
				case 'E':
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
							s_bCountData = false;
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
						s_bRDMA = false;
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
				case 'h':
				case 'H':
				case '?':
					printf("usage:  testframe [switches]\n");
					printf("  -c             counting number test values\n");
					printf("  -dN            N = device number\n");
					printf("  -eX[/Y][/W]    X = test value (hex)  Y = alternate value (hex)  W = width (dwords)\n");
					printf("  -fF/L          F = first frame index L = last frame index\n");
					printf("  -gS            S = frame size\n");
					printf("  -h -?          help\n");
					printf("  -l[NAME]       NAME = log file name (testframe.log)\n");
					printf("  -n             do not use rdma transfers\n");
					printf("  -tN            N = number of test frames\n");
					printf("  -vN            verbose output  N = error count\n");
					printf("  -x             real verbose output (output all)\n");
					printf("\n");
					return 0;
				default:
					printf("invalid switch %c\n", argv[iArg][1]);
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

#ifndef AJA_RDMA						
		if(s_bRDMA)
		{
			printf("error: not built with AJA_RDMA\n");
			throw 0;
		}
#endif						

		if(!s_bUserData)
		{
			s_ulDataWidth = 1;
		}

		if(s_ulDataWidth == 0)
		{
			s_ulDataWidth = 1;
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
		
		int iFrameMax = 0x1000000;
		if (NTV2DeviceCanDo4KVideo(eBoardID))
			iFrameMax = 0x4000000;
		if (NTV2DeviceCanDo8KVideo(eBoardID))
			iFrameMax = 0xc000000;

		int iFrameSize = s_iSystemSize;
		if (iFrameSize > iFrameMax)
			iFrameSize = iFrameMax;

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

		printf("\ntest device %d  start %d  end %d  size %d  count %s\n\n", 
			   s_iBoard, s_iIndexFirst, s_iIndexLast, iFrameSize, sCount);

		if(pLogFile != NULL)
		{
			fprintf(pLogFile, "\ntest device %d  start %d  end %d  size %d  count %s\n\n", 
					s_iBoard, s_iIndexFirst, s_iIndexLast, iFrameSize, sCount);
		}

		ULWord* pSrcBufferAlloc = (ULWord*)new char[iFrameSize + MY_PAGE_SIZE];
		ULWord*	pDstBufferAlloc = (ULWord*)new char[iFrameSize + MY_PAGE_SIZE];
		ULWord*	pZeroBufferAlloc = (ULWord*)new char[iFrameSize + MY_PAGE_SIZE];

		ULWord* pSrcBuffer = (ULWord*)((((ULWord64)pSrcBufferAlloc) + MY_PAGE_SIZE) & MY_PAGE_MASK);
		bSuccess = avCard.DMABufferLock((ULWord*)pSrcBuffer, iFrameSize, true, false);
		if (!bSuccess)
		{
			printf("error - source buffer lock failed\n");
			throw 0;
		}
		
		ULWord* pDstBuffer = (ULWord*)((((ULWord64)pDstBufferAlloc) + MY_PAGE_SIZE) & MY_PAGE_MASK);
		bSuccess = avCard.DMABufferLock((ULWord*)pDstBuffer, iFrameSize, true, false);
		if (!bSuccess)
		{
			printf("error - destination buffer lock failed\n");
			throw 0;
		}

		ULWord* pZeroBuffer = (ULWord*)((((ULWord64)pZeroBufferAlloc) + MY_PAGE_SIZE) & MY_PAGE_MASK);
		bSuccess = avCard.DMABufferLock((ULWord*)pZeroBuffer, iFrameSize, true, false);
		if (!bSuccess)
		{
			printf("error - zero buffer lock failed\n");
			throw 0;
		}

		memset(pZeroBuffer, 0x22, iFrameSize);

#ifdef AJA_RDMA
		int cudaCount = s_iIndexLast - s_iIndexFirst + 1;
		
		if (s_bRDMA)
		{
			pCudaBuffer = new ULWord*[cudaCount];

			for (int i = 0; i < cudaCount; i++)
			{
#ifdef AJA_IGPU
				ce = cudaHostAlloc((void**)&pCudaBuffer[i], iFrameSize, cudaHostAllocDefault);
#else
				ce = cudaMalloc((void**)&pCudaBuffer[i], iFrameSize);
#endif
				if (ce != cudaSuccess) {
					printf("error - allocation of GPU buffer failed: %d\n", ce);
					throw 0;
				}

				cr = cuPointerSetAttribute(&flag, CU_POINTER_ATTRIBUTE_SYNC_MEMOPS,	(CUdeviceptr)pCudaBuffer[i]);
				if (cr != CUDA_SUCCESS) {
					printf("error - cuPointerSetAttribute failed: %d\n", cr);
					throw 0;
				}

				bSuccess = avCard.DMABufferLock((ULWord*)pCudaBuffer[i], iFrameSize, true, true);
				if (!bSuccess)
				{
#ifdef AJA_IGPU
					cudaFreeHost(pCudaBuffer[i]);
#else
					cudaFree(pCudaBuffer[i]);
#endif
					printf("error - GPU buffer lock failed\n");
					throw 0;
				}
			}
		}
#endif		

		int iFrame = 0;
		int iIndex = 0;
		ULWord ulWidth = 0;
		bool bData0 = true;

		if(s_bUserData)
		{
			for(iIndex = 0; iIndex < iFrameSize/4; iIndex++)
			{
				if(s_ulDataCount == 1)
				{
					pSrcBuffer[iIndex] = s_ulData0;
				}
				else
				{
					if(bData0)
					{
						pSrcBuffer[iIndex] = s_ulData0;
					}
					else
					{
						pSrcBuffer[iIndex] = s_ulData1;
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

		int iTest = 0;
		int iError = 0;
		bool bTestFormat = true;
		bool ret;
		while((s_iTestCount == 0) || (iTest < s_iTestCount))
		{
			if(s_bRandomData)
			{
				for(iIndex = 0; iIndex < iFrameSize/4; iIndex++)
				{
					Random(&pSrcBuffer[iIndex]);
				}
			}

			for(iFrame = s_iIndexFirst; iFrame <= s_iIndexLast; iFrame++)
			{
				if(s_bCountData)
				{
					for(iIndex = 0; iIndex < iFrameSize/4; iIndex++)
					{
						pSrcBuffer[iIndex] = s_ulUserCount++;
					}
				}

				//	transfer system memory to frame buffer
				ret = avCard.DMAWrite(iFrame, pSrcBuffer, 0, iFrameSize);
				if (!ret)
				{
					printf("error: dma system to frame buffer failed\n");
					throw 0;
				}

#ifdef AJA_RDMA
				if (s_bRDMA)
				{
					int cudaIdx = iFrame - s_iIndexFirst;
					
					//	transfer frame buffer to gpu memory
					ret = avCard.DMARead(iFrame, pCudaBuffer[cudaIdx], 0, iFrameSize);
					if (!ret)
					{
						printf("error: dma frame buffer to gpu failed\n");
						throw 0;
					}

					//	transfer zero memory to frame buffer
					ret = avCard.DMAWrite(iFrame, pZeroBuffer, 0, iFrameSize);
					if (!ret)
					{
						printf("error: dma zero to frame buffer failed\n");
						throw 0;
					}

					//	transfer gpu to frame buffer
					ret = avCard.DMAWrite(iFrame, pCudaBuffer[cudaIdx], 0, iFrameSize);
					if (!ret)
					{
						printf("error: dma write failed\n");
						throw 0;
					}

					// touch the cuda buffer to prevent low power mode
					ULWord data;
					cudaMemcpy((void*)&data, (void*)pCudaBuffer[cudaIdx], sizeof(ULWord), cudaMemcpyDeviceToHost);
				}
#endif
				
				memset(pDstBuffer, 0xaa, iFrameSize);
				//	transfer from frame buffer to system
				ret = avCard.DMARead(iFrame, pDstBuffer, 0, iFrameSize);
				if (!ret)
				{
					printf("error: dma read failed\n");
					throw 0;
				}
				
				int iECount = 0;
				int iVCount = 0;
				for(iIndex = 0; iIndex < iFrameSize/4; iIndex++)
				{
					if(pSrcBuffer[iIndex] != pDstBuffer[iIndex])
					{
						iECount++;
						bSuccess = false;
						if((iVCount < s_iVerbose) && (s_bVerbose || (pLogFile != NULL)))
						{
							if(bTestFormat)
							{
								if(s_bVerbose)
								{
									printf("\n");
									printf("frm %9d  fb %2d  size %08x\n",
										   iTest, iFrame, iFrameSize);
								}
								if(pLogFile != NULL)
								{
									fprintf(pLogFile, "frm %9d  fb %2d  size %08x\n",
											iTest, iFrame, iFrameSize);
								}
							}
							if(s_bVerbose)
							{
								printf("frm %9d  fb %2d  off %08x  sdx %08x %08x %08x\n", 
									   iTest, iFrame, iIndex*4,
									   pSrcBuffer[iIndex], pDstBuffer[iIndex], 
									   pSrcBuffer[iIndex]^pDstBuffer[iIndex]);
							}
							if(pLogFile != NULL)
							{
								fprintf(pLogFile, "frm %9d  fb %2d  off %08x  src %08x  dst %08x  xor %08x\n", 
										iTest, iFrame, iIndex*4,
										pSrcBuffer[iIndex], pDstBuffer[iIndex], 
										pSrcBuffer[iIndex]^pDstBuffer[iIndex]);
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
						if(s_bRealVerbose)
						{
							printf("frm %9d  fb %2d  src %08x  dst %08x  xor %08x\n", 
								   iTest, iFrame, 
								   pSrcBuffer[iIndex], pDstBuffer[iIndex], 
								   pSrcBuffer[iIndex]^pDstBuffer[iIndex]);
						}
					}
				}

				if(iECount > 0)
				{
					iError++;
					if(s_bVerbose)
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

				printf("frames/errors %d/%d\r", iTest, iError);
				fflush(stdout);
				bTestFormat = true;

				if((s_iTestCount != 0) && (iTest >= s_iTestCount))
				{
					break;
				}
			}
		}

		if(bTestFormat)
		{
			printf("\n");
		}

		if(pLogFile != NULL)
		{
			fprintf(pLogFile, "\ntested %d frames\n", iTest);
			fclose(pLogFile);
			pLogFile = NULL;
		}

		avCard.DMABufferUnlockAll();
			
#ifdef AJA_RDMA
		if (pCudaBuffer != NULL)
		{
			for (int i = 0; i < cudaCount; i++)
			{
#ifdef AJA_IGPU
				ce = cudaFreeHost(pCudaBuffer[i]);
#else
				ce = cudaFree(pCudaBuffer[i]);
#endif
				if (ce != cudaSuccess)
				{
					printf("error - free of GPU buffer failed: %d\n", ce);
				}
			}
			
			delete pCudaBuffer;
		}
#endif
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
