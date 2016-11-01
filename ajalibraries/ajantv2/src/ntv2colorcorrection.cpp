/**
	@file		ntv2colorcorrection.cpp
	@brief		Implements the CNTV2ColorCorrection class.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/
#if 0
#include "ntv2colorcorrection.h"
#include "math.h"
#include <iostream>
#include <assert.h>
using namespace std;


#if defined (NTV2_DEPRECATE)
	#define	CNTV2COLORCORRECTIONCLASS	CNTV2Card
#else
	#define	CNTV2COLORCORRECTIONCLASS	CNTV2ColorCorrection
#endif


static inline int intClamp (int min, int value, int max)
{
	return value < min ? min : (value > max ? max : value);
}

	
#if !defined (NTV2_DEPRECATE)
	CNTV2ColorCorrection::CNTV2ColorCorrection(UWord boardNumber,bool displayErrorMessage,UWord ulBoardType )
	:   CNTV2Status(boardNumber, displayErrorMessage,  ulBoardType) 
	{
		InitNTV2ColorCorrection ();
	}

	CNTV2ColorCorrection::CNTV2ColorCorrection()
	{
		InitNTV2ColorCorrection ();
	}

	CNTV2ColorCorrection::~CNTV2ColorCorrection()
	{
		FreeNTV2ColorCorrection ();
	}
#endif	//	!defined (NTV2_DEPRECATE)


void CNTV2COLORCORRECTIONCLASS::InitNTV2ColorCorrection (void)
{
	SetupColorCorrectionPointers ();

	for (int color=0;  color < NUM_COLORS;  color++)
	{
		_Gamma [color]	= 1.0;
		_Gain [color]	= 1.0;
		_Offset [color]	= 0.0;
		_pColorCorrectionTable [color] = new UWord [1025];
	}
	_channel = NTV2_CHANNEL1;
}


void CNTV2COLORCORRECTIONCLASS::FreeNTV2ColorCorrection (void)
{
	::memset (_pHWTableBaseAddress, 0, sizeof (_pHWTableBaseAddress));
	for (int color=0;  color < NUM_COLORS;  color++)
		delete [] _pColorCorrectionTable [color];
}


// SetupColorCorrectionPointers()
// Maps addressess of Red,Green and Blue Color Correction Tables.
bool CNTV2COLORCORRECTIONCLASS::SetupColorCorrectionPointers(bool ajamac)
{
	::memset (_pHWTableBaseAddress, 0, sizeof (_pHWTableBaseAddress));
	if (IsOpen ())
	{
		for (int color=0;  color < NUM_COLORS;  color++)
			GetRegisterBaseAddress (512 + (color * 512),  &_pHWTableBaseAddress[color]);

		//	Setup for pingponging banks...
		SetColorCorrectionHostAccessBank (_channel == NTV2_CHANNEL1 ? NTV2_CCHOSTACCESS_CH1BANK1 : NTV2_CCHOSTACCESS_CH2BANK1);

		// Mac apps need to be able to instantiate this class without changing output banks
		if (ajamac == false)
			SetColorCorrectionOutputBank (_channel, 0);
	}

	return true;
}


void CNTV2COLORCORRECTIONCLASS::SetColorCorrectionEnable(bool enable)
{
    if ( IsOpen() )
    {
		if ( enable )
			SetColorCorrectionMode(_channel,NTV2_CCMODE_RGB);
		else
			SetColorCorrectionMode(_channel,NTV2_CCMODE_OFF);		
	}
}


// PingPongColorCorrectionTable
void CNTV2COLORCORRECTIONCLASS::PingPongColorCorrectionTable()
{
    if ( IsOpen() )
    {

		NTV2ColorCorrectionHostAccessBank currentBank;
		GetColorCorrectionHostAccessBank(&currentBank);

		switch (currentBank)
		{
		default:
		case NTV2_CCHOSTACCESS_CH1BANK0:
			SetColorCorrectionHostAccessBank(NTV2_CCHOSTACCESS_CH1BANK1);
			SetColorCorrectionOutputBank(_channel,0);
			break;
		case NTV2_CCHOSTACCESS_CH1BANK1:
			SetColorCorrectionHostAccessBank(NTV2_CCHOSTACCESS_CH1BANK0);
			SetColorCorrectionOutputBank(_channel,1);
			break;
		case NTV2_CCHOSTACCESS_CH2BANK0:
			SetColorCorrectionHostAccessBank(NTV2_CCHOSTACCESS_CH2BANK1);
			SetColorCorrectionOutputBank(_channel,0);
			break;
		case NTV2_CCHOSTACCESS_CH2BANK1:
			SetColorCorrectionHostAccessBank(NTV2_CCHOSTACCESS_CH2BANK0);
			SetColorCorrectionOutputBank(_channel,1);
			break;
		case NTV2_CCHOSTACCESS_CH3BANK0:
			SetColorCorrectionHostAccessBank(NTV2_CCHOSTACCESS_CH1BANK1);
			SetColorCorrectionOutputBank(_channel,0);
			break;
		case NTV2_CCHOSTACCESS_CH3BANK1:
			SetColorCorrectionHostAccessBank(NTV2_CCHOSTACCESS_CH1BANK0);
			SetColorCorrectionOutputBank(_channel,1);
			break;
		case NTV2_CCHOSTACCESS_CH4BANK0:
			SetColorCorrectionHostAccessBank(NTV2_CCHOSTACCESS_CH2BANK1);
			SetColorCorrectionOutputBank(_channel,0);
			break;
		case NTV2_CCHOSTACCESS_CH4BANK1:
			SetColorCorrectionHostAccessBank(NTV2_CCHOSTACCESS_CH2BANK0);
			SetColorCorrectionOutputBank(_channel,1);
			break;
		}
	}
}

void CNTV2COLORCORRECTIONCLASS::SetColorCorrectionValues(ColorCorrectionColor colorChoice,
                                                    double gamma, 
                                                    double gain, 
                                                    double offset)
{
    if ( colorChoice == NUM_COLORS )
    {
        // set them all
        for ( int color=0; color < NUM_COLORS; color++ )
        {
            _Gamma[color] = gamma;
            _Gain[color] = gain;
            _Offset[color] = offset;
        }
    }
    else
    {
        _Gamma[colorChoice] = gamma;
        _Gain[colorChoice] = gain;
        _Offset[colorChoice] = offset;
    }

   
}


void CNTV2COLORCORRECTIONCLASS::SetColorCorrectionGamma (ColorCorrectionColor colorChoice, double gamma)
{
    if (colorChoice == NUM_COLORS)
    {
        // set them all
        for (int color=0;  color < NUM_COLORS;  color++)
            _Gamma[color] = gamma;
    }
    else
        _Gamma[colorChoice] = gamma;
}


void CNTV2COLORCORRECTIONCLASS::SetColorCorrectionGain (ColorCorrectionColor colorChoice, double gain)
{
    if (colorChoice == NUM_COLORS)
    {
        // set them all
        for (int color=0;  color < NUM_COLORS;  color++)
            _Gain[color] = gain;
    }
    else
        _Gain[colorChoice] = gain;
}


void CNTV2COLORCORRECTIONCLASS::SetColorCorrectionOffset (ColorCorrectionColor colorChoice, double offset)
{
    if (colorChoice == NUM_COLORS)
    {
        // set them all
        for (int color=0;  color < NUM_COLORS;  color++)
            _Offset[color] = offset;
    }
    else
        _Offset[colorChoice] = offset;
}


void CNTV2COLORCORRECTIONCLASS::BuildTables()
{

    for ( int color=0; color<NUM_COLORS; color++ )
    {
        // For now only do every 4th entry
        for ( int input = 0; input < 1025; input++ )
        {
            UWord* ccTable = _pColorCorrectionTable[color];

            double temp = 1023.0*pow((double)input/1023.0,1.0/_Gamma[color]);
            temp *= _Gain[color];
            temp += _Offset[color];

            int output = int(temp);
            if ( output > 1023 ) output = 1023;
            if ( output < 0 ) output = 0;
        
            ccTable[input] = output;
//            cout << "Input 0x" << hex << input << " Output 0x" << ccTable[input] << endl;        
        }
    }
}


void CNTV2COLORCORRECTIONCLASS::TransferTablesToHardware()
{
	ULWord temp;
	
	if ( !IsOpen() )
		return;

    for ( int color=0; color<NUM_COLORS; color++ )
    {
        UWord* ccTable = _pColorCorrectionTable[color];
        ULWord* hwTable = _pHWTableBaseAddress[color];

        // For now only do every 4th entry
        for ( int output = 0; output < 1024; output+=2 )
        {
			temp = (ccTable[output+1]<<22) + (ccTable[output]<<6) ;
			hwTable[output/2] = temp;
        }
    }

}

// TransferTablesToBuffer
// ccBuffer needs to be NTV2_COLORCORRECTOR_TABLESIZE bytes long.
// This is suitable to pass to transferutocirculate
void CNTV2COLORCORRECTIONCLASS::TransferTablesToBuffer(ULWord* ccBuffer)
{
	ULWord temp;
	
    for ( int color=0; color<NUM_COLORS; color++ )
    {
        UWord* ccTable = _pColorCorrectionTable[color];

        // For now only do every 4th entry
        for ( int output = 0; output < 1024; output+=2 )
        {
			temp = (ccTable[output+1]<<22) + (ccTable[output]<<6) ;
			ccBuffer[output/2] = temp;
        }
		ccBuffer += 512;
    }

}


// SetTables
// Copy contents of external LUTs (each is double LUT[1024]) to internal tables
void CNTV2COLORCORRECTIONCLASS::SetTables(double *redLUT, double *greenLUT, double *blueLUT)
{
	UWord* ccTable;
	
	if (redLUT != NULL)
	{
		ccTable = _pColorCorrectionTable[RED];
		for (int i = 0; i < 1024; i++)
			ccTable[i] = intClamp(0, (UWord)(redLUT[i] + 0.5), 1023);
	}
	
	if (greenLUT != NULL)
	{
		ccTable = _pColorCorrectionTable[GREEN];
		for (int i = 0; i < 1024; i++)
			ccTable[i] = intClamp(0, (UWord)(greenLUT[i] + 0.5), 1023);
	}
	
	if (blueLUT != NULL)
	{
		ccTable = _pColorCorrectionTable[BLUE];
		for (int i = 0; i < 1024; i++)
			ccTable[i] = intClamp(0, (UWord)(blueLUT[i] + 0.5), 1023);
	}
}


// GetTables
// Copy contents of internal tables to external LUTs (each is double LUT[1024])
void CNTV2COLORCORRECTIONCLASS::GetTables(double *redLUT, double *greenLUT, double *blueLUT)
{
	UWord* ccTable;
	
	if (redLUT != NULL)
	{
		ccTable = _pColorCorrectionTable[RED];
		for (int i = 0; i < 1024; i++)
			redLUT[i] = (double)(ccTable[i]);
	}
	
	if (greenLUT != NULL)
	{
		ccTable = _pColorCorrectionTable[GREEN];
		for (int i = 0; i < 1024; i++)
			greenLUT[i] = (double)(ccTable[i]);
	}
	
	if (blueLUT != NULL)
	{
		ccTable = _pColorCorrectionTable[BLUE];
		for (int i = 0; i < 1024; i++)
			blueLUT[i] = (double)(ccTable[i]);
	}
}


// SetTablesToHardware
// Copy contents of external LUTs (each is double LUT[1024]) to hardware
// Note: assumes host access bits have already been set for proper hardware channel/LUT.
// This is NOT safe to run when AutoCirculate is doing "every frame" ColorCorrection updates!
void CNTV2COLORCORRECTIONCLASS::SetTablesToHardware(double *redLUT, double *greenLUT, double *blueLUT)
{
	ULWord* hwTable;
	ULWord lo, hi, temp;
	
	if (redLUT != NULL)
	{
		hwTable = _pHWTableBaseAddress[RED];
		for (int i = 0; i < 512; i++)
		{
			lo = intClamp(0, (ULWord)(redLUT[2*i]   + 0.5), 1023);
			hi = intClamp(0, (ULWord)(redLUT[2*i+1] + 0.5), 1023);
			temp = (hi << kRegColorCorrectionLUTOddShift) + (lo << kRegColorCorrectionLUTEvenShift);
			hwTable[i] = temp;
		}
	}
	
	if (greenLUT != NULL)
	{
		hwTable = _pHWTableBaseAddress[GREEN];
		for (int i = 0; i < 512; i++)
		{
			lo = intClamp(0, (ULWord)(greenLUT[2*i]   + 0.5), 1023);
			hi = intClamp(0, (ULWord)(greenLUT[2*i+1] + 0.5), 1023);
			temp = (hi << kRegColorCorrectionLUTOddShift) + (lo << kRegColorCorrectionLUTEvenShift);
			hwTable[i] = temp;
		}
	}
	
	if (blueLUT != NULL)
	{
		hwTable = _pHWTableBaseAddress[BLUE];
		for (int i = 0; i < 512; i++)
		{
			lo = intClamp(0, (ULWord)(blueLUT[2*i]   + 0.5), 1023);
			hi = intClamp(0, (ULWord)(blueLUT[2*i+1] + 0.5), 1023);
			temp = (hi << kRegColorCorrectionLUTOddShift) + (lo << kRegColorCorrectionLUTEvenShift);
			hwTable[i] = temp;
		}
	}
}


// GetTablesFromHardware
// Copy hardware Color Correction LUT contents to external LUTs (each is double LUT[1024])
// Note: assumes host access bits have already been set for proper hardware channel and bank.
// This will NOT yield predictable results when AutoCirculate is doing "every frame" ColorCorrection updates!
void CNTV2COLORCORRECTIONCLASS::GetTablesFromHardware(double *redLUT, double *greenLUT, double *blueLUT)
{
	ULWord* hwTable;
	ULWord temp;
	
	if (redLUT != NULL)
	{
		hwTable = _pHWTableBaseAddress[RED];
		for (int i = 0; i < 1024; i += 2)
		{
			temp = hwTable[i/2];
			redLUT[i]   = (temp >> kRegColorCorrectionLUTEvenShift) & 0x3FF;
			redLUT[i+1] = (temp >> kRegColorCorrectionLUTOddShift ) & 0x3FF;
		}
	}
	
	if (greenLUT != NULL)
	{
		hwTable = _pHWTableBaseAddress[GREEN];
		for (int i = 0; i < 1024; i += 2)
		{
			temp = hwTable[i/2];
			greenLUT[i]   = (temp >> kRegColorCorrectionLUTEvenShift) & 0x3FF;
			greenLUT[i+1] = (temp >> kRegColorCorrectionLUTOddShift ) & 0x3FF;
		}
	}
	
	if (blueLUT != NULL)
	{
		hwTable = _pHWTableBaseAddress[BLUE];
		for (int i = 0; i < 1024; i += 2)
		{
			temp = hwTable[i/2];
			blueLUT[i]   = (temp >> kRegColorCorrectionLUTEvenShift) & 0x3FF;
			blueLUT[i+1] = (temp >> kRegColorCorrectionLUTOddShift ) & 0x3FF;
		}
	}
}


ULWord* CNTV2COLORCORRECTIONCLASS::GetHWTableBaseAddress(ColorCorrectionColor colorChoice)
{
	assert( _boardOpened );
    	
    if ( colorChoice < NUM_COLORS )
        return _pHWTableBaseAddress[colorChoice];
    else
        return 0;
}

UWord* CNTV2COLORCORRECTIONCLASS::GetTableBaseAddress(ColorCorrectionColor colorChoice)
{
	assert( _boardOpened );
    
    if ( colorChoice < NUM_COLORS )
        return _pColorCorrectionTable[colorChoice];
    else
        return 0;
}

#if !defined (NTV2_DEPRECATE)
bool CNTV2COLORCORRECTIONCLASS::SetBoard(UWord boardNumber)
{
	CNTV2Card::SetBoard (boardNumber);
	return SetupColorCorrectionPointers ();
}
#endif	//	!defined (NTV2_DEPRECATE)
#endif
