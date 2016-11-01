////////////////////////////////////////////////////////
//
// main.cpp 
//
// Test basic NTV2 RS422 functionality
//
////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <signal.h>
#include <time.h>

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2status.h"
#include "ntv2boardfeatures.h"
#include "ntv2boardscan.h"
#include "ntv2devicescanner.h"
#include "ntv2utils.h"


using namespace std;

#ifdef MSWindows
# include <conio.h>
# pragma warning(disable : 4996)
#endif

#ifdef AJALinux
# define MAX_PATH 1024
# define BYTE unsigned char
# define _getch getchar
# include "ntv2winlinhacks.h"
# include "ntv2linuxpublicinterface.h"
#endif

#define MAX_FIFO_BYTES 1024
#define TIMEOUT_COUNT 30

static int  s_iBoard = 0;
static char s_LogFileName[MAX_PATH];
static bool s_bLog = false;
static bool s_bVerbose = false;
static int  s_iVerbose = 10;
static bool s_bQuiet = false;
static bool s_bOverRead = false;
static bool s_bRandom = false;
static bool s_bSingle = false;
static bool s_bWait = false;
static bool s_bSoftwareFifo = false;
static bool s_bBasicTest = false;
static bool s_bInterruptTest = false;
static bool s_bLoopbackTest = false;
static bool s_bReceiveTest = false;
static bool s_bSendTest = false;
static bool s_bInhibit = false;
static int  s_iTestCount = 0;
static BYTE s_TestDataArray[MAX_FIFO_BYTES];
static bool s_bAll = false;
static int  s_iTxFifoSize = 0;
static int  s_iRxFifoSize = 0;
static FILE* s_pLogFile = NULL;
static int  s_iHexChar = -1;
static int  s_iCharDelay = -1;
static int  s_baudDivisor = 0;

static ULWord s_uRegControl = kRegRS422Control;
static ULWord s_uRegTransmit = kRegRS422Transmit;
static ULWord s_uRegReceive = kRegRS422Receive;

static ULWord s_ulRandomW;
static ULWord s_ulRandomX;
static ULWord s_ulRandomY;
static ULWord s_ulRandomZ;

static NTV2_RS422_BAUD_RATE	s_baudRate		= NTV2_RS422_BAUD_RATE_38400;
static NTV2_RS422_PARITY	s_parity		= NTV2_RS422_ODD_PARITY;
static NTV2Channel			s_uartNumber	= NTV2_CHANNEL1;

void SignalHandler(int signal)
{
	s_iTestCount = (-1);
}

void Random(ULWord* pNumber)
{
	// Marsaglia, George. (2003). Xorshift RNGs

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

bool UartPresent(CNTV2Card* pCard)
{
	ULWord value = 0;

	pCard->ReadRegister(s_uRegControl, &value);
	if(value & BIT(8))
	{
		return true;
	}

	return false;
}

bool UartTxFifoEmpty(CNTV2Card* pCard)
{
	ULWord value = 0;

	pCard->ReadRegister(s_uRegControl, &value);
	if(value & BIT(1))
	{
		return true;
	}

	return false;
}

bool UartTxFifoFull(CNTV2Card* pCard)
{
	ULWord value = 0;

	pCard->ReadRegister(s_uRegControl, &value);
	if(value & BIT(2))
	{
		return true;
	}

	return false;
}

bool UartRxFifoEmpty(CNTV2Card* pCard)
{
	ULWord value = 0;

	pCard->ReadRegister(s_uRegControl, &value);
	if(value & BIT(4))
	{
		return false;
	}

	return true;
}

bool UartRxFifoFull(CNTV2Card* pCard)
{
	ULWord value = 0;

	pCard->ReadRegister(s_uRegControl, &value);
	if(value & BIT(5))
	{
		return true;
	}

	return false;
}

bool UartRxFifoParityError(CNTV2Card* pCard, bool bClear = true)
{
	ULWord value = 0;

	pCard->ReadRegister(s_uRegControl, &value);
	if(value & BIT(6))
	{
		if(bClear)
		{
			pCard->WriteRegister(s_uRegControl, value | BIT(6));
		}
		return true;
	}

	return false;
}

bool UartRxFifoOverrun(CNTV2Card* pCard, bool bClear = true)
{
	ULWord value = 0;

	pCard->ReadRegister(s_uRegControl, &value);
	if(value & BIT(7))
	{
		if(bClear)
		{
			pCard->WriteRegister(s_uRegControl, value | BIT(7));
		}
		return true;
	}

	return false;
}

bool UartTxEnable(CNTV2Card* pCard)
{
	ULWord value = 0;

	pCard->ReadRegister(s_uRegControl, &value);
	if(value & BIT(0))
	{
		return true;
	}

	return false;
}

bool UartTxInhibit(CNTV2Card* pCard)
{
	ULWord value = 0;

	pCard->ReadRegister(s_uRegControl, &value);
	if(value & BIT(9))
	{
		return true;
	}

	return false;
}

bool UartRxEnable(CNTV2Card* pCard)
{
	ULWord value = 0;

	pCard->ReadRegister(s_uRegControl, &value);
	if(value & BIT(3))
	{
		return true;
	}

	return false;
}

bool UartTxEnable(CNTV2Card* pCard, bool bEnable)
{
	ULWord value = 0;

	pCard->ReadRegister(s_uRegControl, &value);
	if(value & BIT(0))
	{
		if(!bEnable)
		{
			pCard->WriteRegister(s_uRegControl, value & ~BIT(0));
		}
	}
	else
	{
		if(bEnable)
		{
			pCard->WriteRegister(s_uRegControl, value | BIT(0));
		}
	}

	return UartTxEnable(pCard);
}

bool UartTxInhibit(CNTV2Card* pCard, bool bEnable)
{
	ULWord value = 0;

	pCard->ReadRegister(s_uRegControl, &value);
	if(value & BIT(9))
	{
		if(!bEnable)
		{
			pCard->WriteRegister(s_uRegControl, value & ~BIT(9));
		}
	}
	else
	{
		if(bEnable)
		{
			pCard->WriteRegister(s_uRegControl, value | BIT(9));
		}
	}

	return UartTxInhibit(pCard);
}

bool UartRxEnable(CNTV2Card* pCard, bool bEnable)
{
	ULWord value = 0;

	pCard->ReadRegister(s_uRegControl, &value);
	if(value & BIT(3))
	{
		if(!bEnable)
		{
			pCard->WriteRegister(s_uRegControl, value & ~BIT(3));
		}
	}
	else
	{
		if(bEnable)
		{
			pCard->WriteRegister(s_uRegControl, value | BIT(3));
		}
	}

	return UartRxEnable(pCard);
}

bool UartWrite(CNTV2Card* pCard, BYTE data, bool bTransmit = true)
{
	ULWord value = (ULWord)data;

	if(!UartTxFifoFull(pCard))
	{
		pCard->WriteRegister(s_uRegTransmit, value, bTransmit?0xffffffff:0x000000ff, 0);
	}
	else
	{
		return false;
	}

	if( s_iCharDelay != -1 )
		Sleep( s_iCharDelay );

	return true;
}

bool WaitTxFifoEmpty(CNTV2Card* pCard)
{
	int iTime;
	for(iTime = 0; iTime < TIMEOUT_COUNT; iTime++)
	{
		if(UartTxFifoEmpty(pCard))
		{
			return true;
		}
		Sleep(10);
	}
	return false;
}

bool EmptyTxFifo(CNTV2Card* pCard)
{
	return WaitTxFifoEmpty(pCard);
}

bool UartRead(CNTV2Card* pCard, BYTE* data)
{
	ULWord value = 0;

	if(!UartRxFifoEmpty(pCard))
	{
		pCard->ReadRegister(s_uRegReceive, &value);
	}
	else
	{
		return false;
	}

	if(data != NULL)
	{
		*data = (BYTE)value;
	}

	return true;
}

bool WaitRxFifoNotEmpty(CNTV2Card* pCard)
{
	int iTime;
	for(iTime = 0; iTime < TIMEOUT_COUNT; iTime++)
	{
		if(!UartRxFifoEmpty(pCard))
		{
			return true;
		}
		Sleep(10);
	}
	return false;
}

bool EmptyRxFifo(CNTV2Card* pCard)
{
	ULWord value = 0;
	BYTE data = 0;
	int iByte = 0;
	for(iByte = 0; iByte < MAX_FIFO_BYTES; iByte++)
	{
		if(!UartRead(pCard, &data))
		{
			return true;
		}
	}

	return false;
}

bool UartReset(CNTV2Card* pCard)
{
	ULWord value = 0;
	BYTE data = 0;

	pCard->WriteRegister(s_uRegControl, BIT(6) | BIT(7));

	Sleep(10);  // wait for transmitting byte to clear

	pCard->ReadRegister(s_uRegControl, &value);

	if(value & BIT(4))
	{
		EmptyRxFifo(pCard);
		pCard->ReadRegister(s_uRegControl, &value);
	}
	if(!(value & BIT(1)))
	{
		EmptyTxFifo(pCard);
		pCard->ReadRegister(s_uRegControl, &value);
	}

	if((value & 0x3ff) != 0x102)
	{
		return false;
	}

	return true;
}

bool UartConfigure(CNTV2Card* pCard)
{
	if( !NTV2BoardCanDoProgrammableRS422(pCard->GetDeviceID()) )
	{
		if( (s_baudRate != NTV2_RS422_BAUD_RATE_38400) || (s_parity != NTV2_RS422_ODD_PARITY) )
		{
			printf("The device does not support a programmable RS422\n");
			return false;
		}

		return true;	// Ok if not programmable, but asking to use default values
	}

	if(!UartReset(pCard))
	{
		printf("error: uart reset failed\n");
		return false;
	}
	if(!UartTxEnable(pCard, true))
	{
		printf("error: tx enable failed\n");
		return false;
	}
	if(!UartRxEnable(pCard, true))
	{
		printf("error: rx enable failed\n");
		return false;
	}

	if( !pCard->SetRS422Parity( s_uartNumber, s_parity ) )
	{
		printf("The call to set parity for channel %d failed\n", s_uartNumber);
		return false;
	}

	if( s_baudDivisor )
	{
		ULWord value = 0;
		pCard->ReadRegister(s_uRegControl, &value);

		value &= ~kRegMaskRS422BaudRate;
		value |= 0x3 << kRegMaskRS422BaudRate;	//	Enable variable baud rate control

		value |= s_baudDivisor << 20;

		if( s_iVerbose )
			printf("Setting variable baud rate to %d baud\n", 80000000 / 16 / s_baudDivisor);

		pCard->WriteRegister(s_uRegControl, value);
	}
	else if( !pCard->SetRS422BaudRate( s_uartNumber, s_baudRate ) )
	{
		printf("The call to set baud rate for channel %d failed\n", s_uartNumber);
		return false;
	}

	return true;
}

bool RunBasicTest(CNTV2Card* pCard)
{
	int		iTest = 0;
	int		iError = 0;
	bool	bError = false;
	int		iByte = 0;
	bool	bThrew = false;

	try
	{
		// determine the tx fifo size
		if(!UartTxEnable(pCard, true))
		{
			printf("error: tx enable failed\n");
			throw 0;
		}

		if(s_bInhibit)
		{
			if(!UartTxInhibit(pCard, true))
			{
				printf("error: tx inhibit enable failed\n");
				throw 0;
			}
		}

		// write data until the fifo reports full
		for(iByte = 0; iByte < MAX_FIFO_BYTES; iByte++)
		{
			if(iByte == 0)
			{
				if(!UartTxFifoEmpty(pCard))
				{
					printf("error: tx fifo should be emtpy\n");
					throw 0;
				}
				if(UartTxFifoFull(pCard))
				{
					printf("error: tx fifo should not be full\n");
					throw 0;
				}
				if(!UartWrite(pCard, iByte, !s_bSoftwareFifo))
				{
					printf("error: uart write byte 0 failed\n");
					throw 0;
				}
			} 
			else if(iByte == 1)
			{
				if(UartTxFifoFull(pCard))
				{
					printf("error: tx fifo should not be full\n");
				}
				if(!UartWrite(pCard, iByte, !s_bSoftwareFifo))
				{
					printf("error: tx write byte 1 failed\n");
					throw 0;
				}
			}
			else
			{
				if(UartTxFifoEmpty(pCard))
				{
					printf("error: tx fifo should not be emtpy\n");
				}
				if(UartTxFifoFull(pCard))
				{
					break;
				}
				if(!UartWrite(pCard, iByte, !s_bSoftwareFifo))
				{
					printf("error: tx write byte %d failed\n", iByte);
					throw 0;
				}
			}
		}

		// if never full there was a problem
		if(iByte >= MAX_FIFO_BYTES)
		{
			printf("error: tx fifo not full after %d bytes written\n", MAX_FIFO_BYTES);
//			throw 0;
		}

		if(s_bSoftwareFifo)
		{
			if(!pCard->WriteRegister(s_uRegTransmit, 0, 0xffffffff, 0))
			{
				printf("error: tx write transmit failed\n");
				throw 0;
			}
			Sleep(20);
		}

		if(s_bInhibit)
		{
			if(UartTxInhibit(pCard, false))
			{
				printf("error: tx inhibit disable failed\n");
				throw 0;
			}
		}

		// make sure the fifo is outputting data
		if(!WaitTxFifoEmpty(pCard))
		{
			printf("error: tx wait for empty timeout\n");
			throw 0;
		}

		if(s_bSoftwareFifo || s_bInhibit)
		{
			s_iTxFifoSize = iByte;
		}
		else
		{
			// since data is output while the fifo is filling the number of bytes
			// written is not the fifo size.  Assume the size is really a power of 2 - 1.
			// Typical firmware implementation uses a 127 byte FIFO
			while((iByte>>s_iTxFifoSize) != 0)
			{
				s_iTxFifoSize++;
			}
			s_iTxFifoSize = (0x1 << (s_iTxFifoSize - 1)) - 1;
		}
		
		if(!s_bQuiet && s_bVerbose)
		{
			printf("tx fifo size = %d\n", s_iTxFifoSize);
		}
		if(s_pLogFile != NULL)
		{
			fprintf(s_pLogFile, "tx fifo size = %d\n", s_iTxFifoSize);
		}

		// determine the size of the rx fifo
		if(!UartConfigure(pCard))
		{
			printf("error: uart configation failed\n");
			throw 0;
		}

		if(!UartRxFifoEmpty(pCard))
		{
			printf("error: rx fifo should be emtpy\n");
			throw 0;
		}

		// output bytes until the read fifo is full
		for(iByte = 0; iByte < MAX_FIFO_BYTES; iByte++)
		{
			if(!UartWrite(pCard, iByte))
			{
				printf("error: uart write byte %d failed\n", iByte);
				throw 0;
			}
			if(!WaitTxFifoEmpty(pCard))
			{
				printf("error: tx wait for empty timeout\n");
				throw 0;
			}
			if(!WaitRxFifoNotEmpty(pCard))
			{
				printf("error: rx fifo not empty timeout\n");
				throw 0;
			}
			if(UartRxFifoFull(pCard))
			{
				break;
			}
		}

		if(iByte >= MAX_FIFO_BYTES)
		{
			printf("error: rx fifo not full after %d bytes received\n", MAX_FIFO_BYTES);
			throw 0;
		}

		s_iRxFifoSize = iByte + 1;

		if(!s_bQuiet && s_bVerbose)
		{
			printf("rx fifo size = %d\n", s_iRxFifoSize);
		}
		if(s_pLogFile != NULL)
		{
			fprintf(s_pLogFile, "rx fifo size = %d\n", s_iRxFifoSize);
		}

		// now test the rx fifo overrun by writing one more byte
		if(!UartWrite(pCard, 0xff))
		{
			printf("error: uart write byte 0xff failed\n");
			throw 0;
		}
		Sleep(10);
		if(!UartRxFifoOverrun(pCard))
		{
			printf("error: uart rx overrun test failed\n");
//			throw 0;
		}
	}
	catch(...)
	{
		bThrew = true;
	}

	return bThrew ? false : true;
}

bool RunInterruptTest(CNTV2Card* pCard)
{
	INTERRUPT_ENUMS	txIntEnum = eUart1Tx;
	INTERRUPT_ENUMS	rxIntEnum = eUart1Rx;
	bool			bThrew = false;
	int				iByte = 0;

	if( s_uartNumber == NTV2_CHANNEL2 )
	{
		txIntEnum = eUart2Tx;
		rxIntEnum = eUart2Rx;
	}

	if(!s_bQuiet)
	{
		printf("\nUART interrupt test - board %d \n", s_iBoard);
	}
	if(s_pLogFile != NULL)
	{
		fprintf(s_pLogFile, "\nUART interrupt test - board %d \n", s_iBoard);
	}

	try
	{
		if(!pCard->SubscribeEvent(txIntEnum))
		{
			printf("error: SubscribeUartTxInterruptEvent failed\n");
			throw 0;
		}
		if(!pCard->SubscribeEvent(rxIntEnum))
		{
			printf("error: SubscribeUartRxInterruptEvent failed\n");
			throw 0;
		}
		if(!pCard->EnableInterrupt(txIntEnum))
		{
			printf("error: EnableUartTxInterrupt failed\n");
			throw 0;
		}
		if(!pCard->EnableInterrupt(rxIntEnum))
		{
			printf("error: EnableUartRxInterrupt failed\n");
			throw 0;
		}

		// check interrupts with a long timeout so we outlast the
		// race between UartWrite(), the ISR and
		// WaitForUartTxInterruptEvent().  Windows envents work
		// differently than the Linux driver wait.  In Linux, the
		// WaitForUartTxInterruptEvent call is really
		// WaitForInterrupt and if the ISR has cleared the
		// interrupt happened bit, then this will timeout

		// setup to test the interrupts
		if(!UartConfigure(pCard))
		{
			printf("error: uart configuration failed\n");
			throw 0;
		}

		Sleep(10);

		// clear the interrupt events (Noop on linux in this context)
#ifdef MSWindows
		pCard->WaitForInterrupt(txIntEnum, 0);
		pCard->WaitForInterrupt(rxIntEnum, 0);
#endif
		// write bytes to trip the interrupts
		for(iByte = 0; iByte < s_iTxFifoSize; iByte++)
		{
			if(!UartWrite(pCard, 0xff))
			{
				printf("error: uart write byte 0xff failed\n");
				throw 0;
			}
		}

		if(pCard->WaitForInterrupt(txIntEnum, 1000))
		{
			printf("tx interrupt works\n");
		}
		else
		{
			printf("error: tx interrupt failed\n");
//			throw 0;
		}

		// setup to test the interrupts
		if(!UartConfigure(pCard))
		{
			printf("error: uart configuration failed\n");
			throw 0;
		}

		Sleep(10);

		// clear the interrupt events (Noop on linux in this context)
#ifdef MSWindows
		pCard->WaitForInterrupt(txIntEnum, 0);
		pCard->WaitForInterrupt(rxIntEnum, 0);
#endif
		// write bytes to trip the interrupts
		for(iByte = 0; iByte < s_iTxFifoSize; iByte++)
		{
			if(!UartWrite(pCard, 0xff))
			{
				printf("error: uart write byte 0xff failed\n");
				throw 0;
			}
		}

		if(pCard->WaitForInterrupt(rxIntEnum, 1000))
		{
			printf("rx interrupt works\n");
		}
		else
		{
			printf("error: rx interrupt failed\n");
//			throw 0;
		}

		// cleanup interrupts
		if(!pCard->DisableInterrupt(txIntEnum))
		{
			printf("error: DisableUartTxInterrupt failed\n");
			throw 0;
		}
		if(!pCard->DisableInterrupt(rxIntEnum))
		{
			printf("error: DisableUartRxInterrupt failed\n");
			throw 0;
		}
		if(!pCard->UnsubscribeEvent(txIntEnum))
		{
			printf("error: UnsubscribeUartTxInterruptEvent failed\n");
			throw 0;
		}
		if(!pCard->UnsubscribeEvent(rxIntEnum))
		{
			printf("error: UnsubscribeUartRxInterruptEvent failed\n");
			throw 0;
		}
	}
	catch(...)
	{
		bThrew = true;
	}

	if(!s_bQuiet)
	{
		if( bThrew )
		{
			printf("Interrupt test threw an error\n");
		}
	}
	if(s_pLogFile != NULL)
	{
		if( bThrew )
		{
			fprintf(s_pLogFile, "Interrupt test threw an error\n");
		}
		else
		{
			fprintf(s_pLogFile, "Interrupt test did not throw an error\n");
		}
	}

	return bThrew ? false : true;
}

bool RunLoopbackTest(CNTV2Card* pCard)
{
	int		iTest = 0;
	int		iError = 0;
	bool	bError = false;
	int		iByte = 0;
	bool	bThrew = false;

	try
	{
		if(!s_bQuiet)
		{
			printf("\nUART loopback test - board %d\n", s_iBoard);
		}
		if(s_pLogFile != NULL)
		{
			fprintf(s_pLogFile, "\nUART loopback test - board %d\n", s_iBoard);
		}

		// Ensure FIFO sizes are measured
		RunBasicTest( pCard );

		// setup to do the data integrity test
		int iFifoSize = s_iTxFifoSize;
		if(s_iRxFifoSize < iFifoSize)
		{
			iFifoSize = s_iRxFifoSize;
		}

		if(!UartConfigure(pCard))
		{
			printf("error: uart configuration failed\n");
			throw 0;
		}

		// do dynamic test
		while((s_iTestCount == 0) || (iTest < s_iTestCount))
		{
			ULWord uRandom = 0;
			BYTE uByte = 0;
			bError = false;

			if(s_bOverRead && (s_iTestCount & 0x1))
			{
				// determine number of bytes to tx
				Random(&uRandom);
				int iTestBytes = (uRandom % (iFifoSize - 1)) + 1;

				// tx random data
				for(iByte = 0; iByte < iTestBytes; iByte++)
				{
					s_TestDataArray[iByte] = (BYTE)iByte;
					if(s_bRandom)
					{
						Random(&uRandom);
						s_TestDataArray[iByte] = (BYTE)uRandom;
					}
					if(!UartWrite(pCard, s_TestDataArray[iByte]))
					{
						if(!s_bQuiet && s_bVerbose)
						{
							printf("error(%d): uart write byte %d failed\n", iTest, iByte);
						}
						if(s_pLogFile != NULL)
						{
							fprintf(s_pLogFile, "error(%d): uart write byte %d failed\n", iTest, iByte);
						}
						bError = true;
						break;
					}
				}

				// overread the rx fifo while transmitting
				for(iByte = 0; iByte < (iTestBytes + 10); iByte++)
				{
					ULWord value = 0;
					pCard->ReadRegister(s_uRegReceive, &value);
				}

				// now try to reset the uart
				if(!UartConfigure(pCard))
				{
					printf("error: uart configuration failed\n");
					throw 0;
				}
			}
			else
			{
				// determine number of bytes to tx
				Random(&uRandom);
				int iTestBytes = (uRandom % (iFifoSize - 1)) + 1;

				if(s_bInhibit)
				{
					if(!UartTxInhibit(pCard, true))
					{
						printf("error: tx inhibit enable failed\n");
						throw 0;
					}
				}

				// tx random data
				for(iByte = 0; iByte < iTestBytes; iByte++)
				{
					s_TestDataArray[iByte] = (BYTE)iByte;
					if(s_bRandom)
					{
						Random(&uRandom);
						s_TestDataArray[iByte] = (BYTE)uRandom;
					}
					if(!UartWrite(pCard, s_TestDataArray[iByte], !s_bSoftwareFifo || (iByte == (iTestBytes-1))))
					{
						if(!s_bQuiet && s_bVerbose)
						{
							printf("error(%d): uart write byte %d failed\n", iTest, iByte);
						}
						if(s_pLogFile != NULL)
						{
							fprintf(s_pLogFile, "error(%d): uart write byte %d failed\n", iTest, iByte);
						}
						bError = true;
						break;
					}
				}

				if(s_bSoftwareFifo)
				{
					Sleep(20);
				}

				if(s_bInhibit)
				{
					if(!UartRxFifoEmpty(pCard))
					{
						printf("error: rx fifo not empty\n");
					}
					if(UartTxInhibit(pCard, false))
					{
						printf("error: tx inhibit disable failed\n");
						throw 0;
					}
				}

				// wait for data transfer out of tx fifo
				if(!bError && !WaitTxFifoEmpty(pCard))
				{
					if(!s_bQuiet && s_bVerbose)
					{
						printf("error(%d): tx wait for fifo empty timeout\n", iTest);
					}
					if(s_pLogFile != NULL)
					{
						fprintf(s_pLogFile, "error(%d): tx wait for fifo empty timeout\n", iTest);
					}
					bError = true;
				}

				// wait for data to be received by rx fifo
				if(!bError && !WaitRxFifoNotEmpty(pCard))
				{
					if(!s_bQuiet && s_bVerbose)
					{
						printf("error(%d): rx fifo not empty timeout\n", iTest);
					}
					if(s_pLogFile != NULL)
					{
						fprintf(s_pLogFile, "error(%d): rx fifo not empty timeout\n", iTest);
					}
					bError = true;
				}

				// check for tx/rx errors
				if(!bError && UartRxFifoParityError(pCard))
				{
					if(!s_bQuiet && s_bVerbose)
					{
						printf("error(%d): rx fifo parity error\n", iTest);
					}
					if(s_pLogFile != NULL)
					{
						fprintf(s_pLogFile, "error(%d): rx fifo parity error\n", iTest);
					}
					bError = true;
				}
				if(!bError && UartRxFifoOverrun(pCard))
				{
					if(!s_bQuiet && s_bVerbose)
					{
						printf("error(%d): rx fifo overrun error\n", iTest);
					}
					if(s_pLogFile != NULL)
					{
						fprintf(s_pLogFile, "error(%d): rx fifo overrun error\n", iTest);
					}
					bError = true;
				}

				Sleep(10);  // wait for transmitting byte to clear

				if(!bError)
				{
					int iVCount = 0;
					// read data from rx fifo
					for(iByte = 0; iByte < iTestBytes; iByte++)
					{
						if(!UartRead(pCard, &uByte))
						{
							if(!s_bQuiet && s_bVerbose)
							{
								printf("error(%d): uart read byte %d failed\n", iTest, iByte);
							}
							if(s_pLogFile != NULL)
							{
								fprintf(s_pLogFile, "error(%d): uart read byte %d failed\n", iTest, iByte);
							}
							bError = true;
							break;
						}

						// compare with rx with tx data
						if(uByte != s_TestDataArray[iByte])
						{
							if(!s_bQuiet && (s_bVerbose || s_bAll))
							{
								printf("error(%d): data byte %d  write %02x  read %02x  size %d\n", 
									iTest, iByte, s_TestDataArray[iByte], uByte, iTestBytes);
							}
							if(s_pLogFile != NULL)
							{
								fprintf(s_pLogFile, "error(%d): data byte %d  write %02x  read %02x  size %d\n", 
									iTest, iByte, s_TestDataArray[iByte], uByte, iTestBytes);
							}
							bError = true;
							if(iVCount >= s_iVerbose)
							{
								break;
							}
							if(!s_bAll)
							{
								iVCount++;
							}
						}
						else
						{
							if(!s_bQuiet && s_bAll)
							{
								printf("valid(%d): data byte %d  write %02x  read %02x  size %d\n", 
									iTest, iByte, s_TestDataArray[iByte], uByte, iTestBytes);
							}
						}
					}
				}
			}

			iTest++;
			if(bError)
			{
				// if error then reset uart
				iError++;
				if(s_pLogFile != NULL)
				{
					fflush(s_pLogFile);
				}
				if(!UartConfigure(pCard))
				{
					printf("error: uart configuration failed\n");
					throw 0;
				}
			}

			if(!s_bQuiet)
			{
				printf("tests/errors %d/%d\r", iTest, iError);
				fflush(stdout);
			}

			if((s_iTestCount != 0) && (iTest > s_iTestCount))
			{
				break;
			}
		}
	}
	catch(...)
	{
		bThrew = true;
	}

	if(!s_bQuiet)
	{
		if( bThrew )
		{
			printf("Loopback test threw an error\n");
		}
	}
	if(s_pLogFile != NULL)
	{
		if( bThrew )
		{
			fprintf(s_pLogFile, "Loopback test threw an error\n");
		}
		else
		{
			fprintf(s_pLogFile, "Loopback test did not throw an error\n");
		}
		fprintf(s_pLogFile, "tested %d tx/rx\n\n", iTest);

	}

	return bThrew ? false : true;
}

bool RunReceiveTest(CNTV2Card* pCard)
{
	int		iTest = 0;
	BYTE	uByte = 0;;
	bool	bThrew = false;

	try
	{
		if(!s_bQuiet)
		{
			printf("\nUART receive test - board %d\n", s_iBoard);
		}
		if(s_pLogFile != NULL)
		{
			fprintf(s_pLogFile, "\nUART receive test - board %d\n", s_iBoard);
		}

		if(!UartConfigure(pCard))
		{
			printf("error: uart configuration failed\n");
			throw 0;
		}

		// Print any chars that come in
		while((s_iTestCount == 0) || (iTest < s_iTestCount))
		{
			if(!WaitRxFifoNotEmpty(pCard))
			{
				continue;	// Wait forever
			}

			if(!UartRead(pCard, &uByte))
			{
				printf("error: uart read failed\n");
				throw 0;
			}

			printf("%c", uByte);
			fflush(stdout);

			iTest++;
		}
	}
	catch(...)
	{
		bThrew = true;
	}

	if(!s_bQuiet)
	{
		if( bThrew )
		{
			printf("Receive test threw an error\n");
		}
	}
	if(s_pLogFile != NULL)
	{
		if( bThrew )
		{
			fprintf(s_pLogFile, "Receive test threw an error\n");
		}
		else
		{
			fprintf(s_pLogFile, "Receive test did not throw an error\n");
		}
	}

	return bThrew ? false : true;
	return true;
}

bool RunSendTest(CNTV2Card* pCard)
{
	int		iTest = 0;
	int		iByte = 'A';
	bool	bThrew = false;

	try
	{
		if(!s_bQuiet)
		{
			printf("\nUART send test - board %d\n", s_iBoard);
		}
		if(s_pLogFile != NULL)
		{
			fprintf(s_pLogFile, "\nUART send test - board %d\n", s_iBoard);
		}

		if(!UartConfigure(pCard))
		{
			printf("error: uart configuration failed\n");
			throw 0;
		}

		if( s_iHexChar != -1 )
			iByte = s_iHexChar;

		// Send the alphabet
		while((s_iTestCount == 0) || (iTest < s_iTestCount))
		{
			if(UartTxFifoFull(pCard))
			{
				Sleep(10);
			}

			if(UartTxFifoFull(pCard))
			{
				printf("error: tx fifo should not be full\n");
				throw 0;
			}

			if(!UartWrite(pCard, iByte, !s_bSoftwareFifo))
			{
				printf("error: tx write byte %d failed\n", iByte);
				throw 0;
			}

			iTest++;

			if( s_iHexChar == -1 )
				iByte = (iByte == 'Z') ? 'A' : iByte + 1;
		}
	}
	catch(...)
	{
		bThrew = true;
	}

	if(!s_bQuiet)
	{
		if( bThrew )
		{
			printf("Send test threw an error\n");
		}
	}
	if(s_pLogFile != NULL)
	{
		if( bThrew )
		{
			fprintf(s_pLogFile, "Send test threw an error\n");
		}
		else
		{
			fprintf(s_pLogFile, "Send test did not throw an error\n");
		}
	}

	return bThrew ? false : true;
}

int main(int argc, char* argv[])
{
	int iData0 = 0;
	int iData1 = 0;
	int iSlot = 0;
	ULWord ulDataCount = 0;
	ULWord ulData0 = 0;
	ULWord ulData1 = 0;
	bool bParseError = false;
	bool bMagma = false;

	try
	{
		signal(SIGINT, SignalHandler);

		srand((unsigned)time(NULL));	
		s_ulRandomW = rand();
		s_ulRandomX = rand();
		s_ulRandomY = rand();
		s_ulRandomZ = rand();
		strcpy(s_LogFileName, "qauart.log");

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
						s_bAll = true;
						break;
					}
				case 'b':
				case 'B':
					{
						iData0 = 0;
						if(sscanf(&argv[iArg][2], "%d", &iData0) == 1)
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
				case 'c':
				case 'C':
					{
						s_bSoftwareFifo = true;
						break;
					}
				case 'd':
				case 'D':
					{
						iData0 = 0;
						if(sscanf(&argv[iArg][2], "%d", &iData0) == 1)
						{
							s_iCharDelay = iData0;
						}
						else
						{
							printf("error: missing character delay value\n");
							bParseError = true;
						}
						break;
					}
				case '2':
					{
						s_uRegControl = kRegRS4222Control;
						s_uRegTransmit = kRegRS4222Transmit;
						s_uRegReceive = kRegRS4222Receive;
						s_uartNumber = NTV2_CHANNEL2;
						break;
					}
				case 'f':
				case 'F':
					{
						iData0 = 0;
						if(sscanf(&argv[iArg][2], "%d", &iData0) == 1)
						{
							if((iData0 > 0) && (iData0 < 256))
							{
								s_baudDivisor = iData0;
								break;
							}
							else
								s_baudDivisor = 0;

							switch(iData0)
							{
							case 9600:
								s_baudRate = NTV2_RS422_BAUD_RATE_9600;
								break;
							case 19200:
								s_baudRate = NTV2_RS422_BAUD_RATE_19200;
								break;
							case 38400:
								s_baudRate = NTV2_RS422_BAUD_RATE_38400;
								break;
							default:
								printf("error: a bit frequency (baud rate) of %d is not supported\n", iData0);
								bParseError = true;
								break;
							}
						}
						else
						{
							printf("error: missing bit frequency (baud rate)\n");
							bParseError = true;
						}
						break;
					}
				case 'g':
				case 'G':
					{
						iData0 = 0;
						if(sscanf(&argv[iArg][2], "%c", &iData0) == 1)
						{
							switch(iData0)
							{
							case 'o':
							case 'O':
								s_parity = NTV2_RS422_ODD_PARITY;
								break;
							case 'e':
							case 'E':
								s_parity = NTV2_RS422_EVEN_PARITY;
								break;
							case 'n':
							case 'N':
								s_parity = NTV2_RS422_NO_PARITY;
								break;
							default:
								printf("error: a parity of %c is not supported\n", iData0);
								bParseError = true;
								break;
							}
						}
						else
						{
							printf("error: missing parity type\n");
							bParseError = true;
						}
						break;
					}
				case 'i':
				case 'I':
					{
						s_bInterruptTest = true;
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
				case 'm':
				case 'M':
					{
						iData0 = 0;
						if(sscanf(&argv[iArg][2], "%d", &iData0) == 1)
						{
							iSlot = iData0;
							bMagma = true;
						}
						else
						{
							printf("error: missing Magma number\n");
							bParseError = true;
						}
						break;
					}
				case 'o':
				case 'O':
					{
						s_bOverRead = true;
						break;
					}
				case 'p':
				case 'P':
					{
						s_bInhibit = true;
						break;
					}
				case 'r':
				case 'R':
					{
						s_bRandom = true;
						break;
					}
				case 's':
				case 'S':
					{
						s_bSingle = true;
						break;
					}
				case 't':
				case 'T':
					{
						iData0 = 0;
						if(sscanf(&argv[iArg][2], "%d", &iData0) == 1)
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
						iData0 = 0;
						if(sscanf(&argv[iArg][2], "%c", &iData0) == 1)
						{
							switch(iData0)
							{
							case 'b':
							case 'B':
								s_bBasicTest = true;
								break;
							case 'i':
							case 'I':
								s_bInterruptTest = true;
								break;
							case 'l':
							case 'L':
								s_bLoopbackTest = true;
								break;
							case 'r':
							case 'R':
								s_bReceiveTest = true;
								break;
							case 's':
							case 'S':
								s_bSendTest = true;
								break;
							default:
								printf("error: the uart test %c is unknown\n", iData0);
								bParseError = true;
								break;
							}
						}
						else
						{
							printf("error: missing uart test type\n");
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
				case 'q':
				case 'Q':
					{
						s_bQuiet = true;
						break;
					}
				case 'w':
				case 'W':
					{
						s_bWait = true;
						break;
					}
				case 'x':
				case 'X':
					{
						iData0 = 0;
						if(sscanf(&argv[iArg][2], "%x", &iData0) == 1)
						{
							s_iHexChar = iData0;
						}
						else
						{
							printf("error: hex value missingt\n");
							bParseError = true;
						}
						break;
					}
				case 'h':
				case 'H':
				case '?':
					printf("usage:  qauart [switches]\n");
					printf("  -a           print all results\n");
					printf("  -bN          N = board number\n");
					printf("  -c           software fifo\n");
					printf("  -dN          N = millisecond delay between sending characters\n");
					printf("  -fN          N = frequency of bits: 9600, 19200, or 38400\n");
					printf("                   or (27 / 16) MHz divisor between 1 and 255\n");
					printf("  -gC          C = generate parity of type 'O'dd, 'E'ven, or 'N'one\n");
					printf("  -h -?        help\n");
					printf("  -i           test uart interrupt\n");

					printf("  -l[NAME]     NAME = log file name (qauart.log)\n");
					printf("  -mN          N = Magma chassis(x) slot(y): xy\n");
					printf("  -o           overread the rx fifo between each data test\n");
					printf("  -p           test tx inhibit\n");
					printf("  -q           quiet output (batch)\n");
					printf("  -tN          N = number of uart tests\n");
					printf("  -r           random data\n");
					printf("  -s           single byte tx/rx\n");
					printf("  -uC          C = uart test to run\n");
					printf("                  B for basic test\n");
					printf("                  I for interrupt test\n");
					printf("                  L for loopback test\n");
					printf("                  R for receive test\n");
					printf("                  S for send test\n");
					printf("  -vN          verbose output  N = error count\n");
					printf("  -w           wait for user before exit\n");
					printf("  -xN          N = hex character to repeat in the send test\n");
					printf("  -2           test 2nd uart\n");
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

		if(s_bLog)
		{
			s_pLogFile = fopen(s_LogFileName, "a");
			if(s_pLogFile == NULL)
			{
				printf("error: can not open log file %s\n", s_LogFileName);
				throw 0;
			}
		}

		int boardNumber = 0;
		if(bMagma)
		{
			CNTV2DeviceScanner ntv2BoardScan;
			NTV2DeviceInfoList& boardList = ntv2BoardScan.GetDeviceInfoList();
			NTV2DeviceInfoList::iterator boardIter;

			for( boardIter = boardList.begin(); boardIter != boardList.end(); boardIter++)
			{
				NTV2DeviceInfo info = *boardIter;
				if(info.pciSlot != iSlot)
				{
					continue;
				}
				else
				{
					boardNumber = info.deviceIndex;
				}
			}
		}
		else
		{
			// find the board
			NTV2DeviceInfo boardInfo;
			CNTV2DeviceScanner ntv2BoardScan;
			if(ntv2BoardScan.GetNumDevices() <= (ULWord)s_iBoard)
			{
				printf("error: opening board %d failed\n", s_iBoard);
				throw 0;
			}
			boardInfo = ntv2BoardScan.GetDeviceInfoList()[s_iBoard];
			boardNumber = boardInfo.deviceIndex;
		}
		CNTV2Card avCard(boardNumber, false);

		if(avCard.IsOpen() == false)
		{
			printf("error: opening board %d failed\n", s_iBoard);
			throw 0;
		}
		NTV2DeviceID eBoardID = avCard.GetDeviceID();

#if defined(MSWindows)
		avCard.WriteRegister(kRegSoftwareUartFifo, s_bSoftwareFifo? 1 : 0);
#endif

		// find the uart and reset it
		if(!UartPresent(&avCard))
		{
			printf("error: no uart found\n");
			throw 0;
		}

		if(!UartConfigure(&avCard))
		{
			printf("error: uart configuration failed\n");
			throw 0;
		}

		//
		// Dispatch the requested tests
		//
		if( s_bBasicTest )
		{
			RunBasicTest( &avCard );
		}
		if( s_bInterruptTest )
		{
			RunInterruptTest( &avCard );
		}
		if( s_bLoopbackTest )
		{
			RunLoopbackTest( &avCard );
		}
		if( s_bReceiveTest )
		{
			RunReceiveTest( &avCard );
		}
		if( s_bSendTest )
		{
			RunSendTest( &avCard );
		}

		if(!UartConfigure(&avCard))
		{
			printf("error: uart configuration failed\n");
			throw 0;
		}
#if defined(MSWindows)
		avCard.WriteRegister(kRegSoftwareUartFifo, 0);
#endif
		if(!s_bQuiet)
		{
			printf("\n\n");
		}

		if(s_pLogFile != NULL)
		{
			fclose(s_pLogFile);
			s_pLogFile = NULL;
		}

		if(s_bWait)
		{
			printf("\npress any key to exit\n");
			_getch();
		}

		return 0;
	}
	catch(...)
	{
	}

	if(s_bWait)
	{
		printf("\npress any key to exit\n");
		_getch();
	}

	return -1;
}
