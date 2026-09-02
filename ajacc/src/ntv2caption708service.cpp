/**
	@file		ntv2caption708service.cpp
	@brief		Implementation of the CNTV2Caption708Service class.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc. All rights reserved.
**/


#include "ntv2caption708service.h"
#include "ajabase/system/debug.h"


#define	LOGMYERROR(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Service, AJA_DebugSeverity_Error,	GetLogLabel() << ":  " << AJAFUNC << ":  " << __xpr__)
#define	LOGMYINFO(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Service, AJA_DebugSeverity_Info,		GetLogLabel() << ":  " << AJAFUNC << ":  " << __xpr__)
#define	LOGMYDEBUG(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Service, AJA_DebugSeverity_Debug,	GetLogLabel() << ":  " << AJAFUNC << ":  " << __xpr__)


using namespace std;


#if defined (MSWindows)
	#pragma warning(disable: 4800) 
	#pragma warning(disable:4127)
#endif

static unsigned gInstanceTally	(0);




/////////////////////////////////////////////////////////////////////////////
// Constructor
//
CNTV2Caption708Service::CNTV2Caption708Service (void)
{
	++gInstanceTally;
	InitService (0);

}	//	constructor


CNTV2Caption708Service::~CNTV2Caption708Service ()
{
}	//	destructor


// InitService()
//		Clear the CNTV2ServiceInfo struct	
//
void CNTV2Caption708Service::InitService (const int inServiceIndex)
{
	ostringstream	oss;	oss << "Caption708Service-" << gInstanceTally << "-" << inServiceIndex;
	SetLogLabel(oss.str());

	mCurrentWindow = NTV2_CC708WindowIDMin;

	mServiceInfo.bSvcActive	   = false;								// default = not active
	mServiceInfo.captionSvcNumber = inServiceIndex;					// service number (Line 21 data = 0; 708 data = 1 - 63)

	//	From caption service descriptor: ATSC A/65 pg 71
	mServiceInfo.language   = NTV2_CC708SvcLang_English;			// default = English
	mServiceInfo.digitalCC  = (inServiceIndex == 0 ? false : true);	// use index 0 for Line 21 captions, all the rest are 708
	mServiceInfo.easyReader = false;
	mServiceInfo.wideAspect = false;

	mServiceBlockQueue.Flush();

	for (int i(0);  i < NTV2_CC708NumWindows;  i++)
		mWindowArray[i].InitWindow(i);

	mServiceBlockQueue.SetDebugChannel (static_cast <int> (NTV2_CC608_ChannelMax + inServiceIndex));

}	//	InitService


// SetServiceInfo()
//
bool CNTV2Caption708Service::SetServiceInfo (const NTV2_CC708ServiceInfo & inNewSvcInfo)
{
	mServiceInfo = inNewSvcInfo;	// struct copy
	mServiceBlockQueue.SetDebugChannel (static_cast <int> (NTV2_CC608_ChannelMax + mServiceInfo.captionSvcNumber));
	return true;

}	//	SetServiceInfo


// GetCommandSize()
//
size_t CNTV2Caption708Service::GetCommandSize (const UByte * pData, const size_t dataSize) const
{
	size_t	result	(0);

	//	Sanity check...
	if (pData == NULL || dataSize <= 0)
		return result;

	//	The size of most commands can be determined based on the first word...
	UByte cmd1 = pData[0];

	//	If the first byte is the "EXT1" character (0x10),then we have to look at (at least) the 2nd byte to determine the total size...
	UByte cmd2 = 0;
	UByte cmd3 = 0;
	if (cmd1 == 0x10)
	{
		//	Before getting the 2nd byte, make sure there is one...
		if (dataSize > 1)
		{
			cmd2 = pData[1];

			//	If 0x90 <= cmd2 <= 0x9f, then this is a "variable-length command" (see CEA-708C Section 7.1.11.2),
			//	which means the true length is in the third byte...
			if (cmd2 >= 0x90 && cmd2 <= 0x9f)
			{
				//	Before getting the 3rd byte, make sure there is one...
				if (dataSize > 2)
					cmd3 = pData[2];
				else
					return 0;	//	Not enough data to support a 3-byte command
			}
		}
		else
			return 0;	//	Not enough data to support a 2-byte command
	}

	//	There are four standard "Code Groups" (CL, GL, CR, and GR), determined by ranges of the 8-bit data:
	//
	//		 Data Range  Code		Standard	Extended
	//					 Group		  Set		  Set
	//		0x00 - 0x1F:  CL		  C0		  C2
	//		0x20 - 0x7F:  GL		  G0		  G2
	//		0x80 - 0x9F:  CR		  C1		  C3
	//		0xA0 - 0xFF:  GR		  G1		  G3
	//
	//	Each Code Group contains two "Code Sets", "Standard" and "Extended". The Extended Code Sets are accessed by first
	//	transmitting an EXT1 byte (0x10). Without this byte, the Standard Code Sets are assumed.

	bool bExtended = (cmd1 == 0x10);

	if (!bExtended)
	{
		//	Standard Code Set
		if (/*cmd1 >= 0x00 &&*/ cmd1 <= 0x1F)			// Code Set C0 - multi-byte commands
		{
				// This is further broken into 3 sections:
				//		0x00 - 0x0F are 1-byte codes
				//		0x10 - 0x17 are 2-byte codes
				//		0x18 - 0x1F are 3-byte codes
			if (/*cmd1 >= 0x00 &&*/ cmd1 <= 0x0F)			// 1-byte command
				result = 1;
			else if (cmd1 >= 0x10 && cmd1 <= 0x17)			// 2-byte command
				result = 2;
			else // if (cmd1 >= 0x18 && cmd1 <= 0x1F)		// 3-byte command
				result = 3;
		}
		else if (cmd1 >= 0x20 && cmd1 <= 0x7F)			// Code Set G0 - all 1 char
			result = 1;
		else if (cmd1 >= 0x80 && cmd1 <= 0x9F)			// Code Set C1 - length depends on length of specific command
		{
			switch (cmd1)
			{
				case 0x80:									// SetCurrentWindow command (no extra params)
				case 0x81:
				case 0x82:
				case 0x83:
				case 0x84:
				case 0x85:
				case 0x86:
				case 0x87:	result = 1;	break;

				case 0x88:	result = 2;	break;				// ClearWindows command - next byte is bitmap of windows to clear
				case 0x89:	result = 2;	break;				// DisplayWindows command - next byte is bitmap of windows to display
				case 0x8a:	result = 2;	break;				// HideWindows command - next byte is bitmap of windows to hide
				case 0x8b:	result = 2;	break;				// ToggleWindows command - next byte is bitmap of windows to toggle
				case 0x8c:	result = 2;	break;				// DeleteWindows command - next byte is bitmap of windows to delete
				case 0x8d:	result = 2;	break;				// Delay command - next byte is delay timeout (in tenths of seconds)
				case 0x8e:	result = 1;	break;				// DelayCancel command - no params
				case 0x8f:	result = 1;	break;				// Reset command - no params

				case 0x90:	result = 3;	break;				// SetPenAttributes command - next 2 bytes are params
				case 0x91:	result = 4;	break;				// SetPenColor command - next 3 bytes are params
				case 0x92:	result = 3;	break;				// SetPenLocation command - next 2 bytes are params

				case 0x93:									// reserved (undefined) - no params
				case 0x94:	
				case 0x95:	
				case 0x96:	result = 1;	break;	

				case 0x97:	result = 5;	break;				// SetWindowAttributes command - next 4 bytes are params

				case 0x98:									// DefineWindow command - next 6 bytes are params
				case 0x99:	
				case 0x9a:	
				case 0x9b:	
				case 0x9c:	
				case 0x9d:	
				case 0x9e:	
				case 0x9F:	result = 7;	break;
			}
		}
		else  // (cmd1 >= 0xA0 && cmd1 <= 0xFF)			// Code Set G1 - all 1 char
			result = 1;

	}	// if (!bExtended)
	else
	{
		//	The first code was EXT (0x10), so this is the "Extended" Set
		//	The length of the command depends on the 2nd command byte: most are only a single extra byte (total = 2),
		//	but some (Code Sets C2 and C3) are "extensions to extensions"

		if (/* cmd2 >= 0x00 &&*/ cmd2 <= 0x1f)			// Code Set C2 - length depends on cmd2 (see CEA-708C Section 7.1.10)
		{
			if (/* cmd2 >= 0x00 &&*/ cmd2 <= 0x07)			// EXT + cmd2
				result = 2;
			else if (cmd2 >= 0x08 && cmd2 <= 0x0f)			// EXT + cmd2 + 1 extra bytes
				result = 3;
			else if (cmd2 >= 0x10 && cmd2 <= 0x17)			// EXT + cmd2 + 2 extra bytes
				result = 4;
			else /* (cmd2 >= 0x18 && cmd2 <= 0x1f) */		// EXT + cmd2 + 3 extra bytes
				result = 5;
		}
		else if (cmd2 >= 0x20 && cmd2 <= 0x7f)			// Code Set G2 - all EXT + 1 char
			result = 2;
		else if (cmd2 >= 0x80 && cmd2 <= 0x9f)			// Code Set C3 - length depends on cmd2 (see CEA-708C Section 7.1.11)
		{
			if (cmd2 >= 0x80 && cmd2 <= 0x87)			// EXT + cmd2 + 4 extra bytes
				result = 6;
			else if (cmd2 >= 0x88 && cmd2 <= 0x8f)			// EXT + cmd2 + 5 extra bytes
				result = 7;
			else /* (cmd2 >= 0x90 && cmd2 <= 0x9f) */		// "variable-length command" - the length is in the cmd3 (EXT + cmd2 + cmd3 + N extra bytes)
				result = 3 + (cmd3 & 0x1f);
		}
		else /* (cmd2 >= 0xa0 && cmd2 <= 0xff) */		// Code Set G3 - all EXT + 1 char
			result = 2;
	}

	return result;	

}	//	GetCommandSize


// ParseCommand()
//
//	Parse the 708 command that starts at pData[0].
//	Note that dataSize is NOT necessarily the size of the command we're parsing - it's only
//	an indication of how much valid data is in pData so we don't walk off the end of an array.
//	Returns the size of the command, or zero if error in parsing.
//
size_t CNTV2Caption708Service::Parse708Command (const UByte * pData, const size_t dataSize)
{
	//	Reality check...
	if (pData == NULL || dataSize == 0)
	{
		LOGMYERROR("Bad params");
		return 0;
	}

	size_t	cmdLength	(GetCommandSize (pData, dataSize));
	if (cmdLength > 0)
	{
		//	There are four standard "Code Groups" (CL, GL, CR, and GR), determined by ranges of the 8-bit data:
		//
		//		 Data Range  Code		Standard	Extended
		//					 Group		  Set		  Set
		//		0x00 - 0x1F:  CL		  C0		  C2
		//		0x20 - 0x7F:  GL		  G0		  G2
		//		0x80 - 0x9F:  CR		  C1		  C3
		//		0xA0 - 0xFF:  GR		  G1		  G3
		//
		//	Each Code Group contains two "Code Sets", "Standard" and "Extended". The Extended Code Sets are accessed by first
		//	transmitting an EXT1 byte (0x10). Without this byte, the Standard Code Sets are assumed.

		//	See if this is an EXT1 byte
		UByte cmd1 = pData [0];	// shortcut
		bool bExtended = (cmd1 == 0x10);

		if (!bExtended)
		{
			//	Standard Code Set
			if (/*cmd1 >= 0x00 &&*/ cmd1 <= 0x1F)		// Code Set C0
			{
				//	This is further broken into 3 sections:
				//		0x00 - 0x0F are 1-byte codes
				//		0x10 - 0x17 are 2-byte codes
				//		0x18 - 0x1F are 3-byte codes

				if (/*cmd1 >= 0x00 &&*/ cmd1 <= 0x0F)				// 1-byte codes
				{
					switch (cmd1)
					{
						case 0x00:				break;		// NULL - ignore
						case 0x03:	DoETX();	break;		// ETX
						case 0x08:	DoBS();		break;		// BackSpace (BS)
						case 0x0C:	DoFF();		break;		// FormFeed (FF)
						case 0x0D:	DoCR();		break;		// Carriage Return (CR/LF)
						case 0x0E:	DoHCR();	break;		// Horizontal Carriage Return (CR)
						default:				break;		// undefined - ignore
					}
				}
				else if (cmd1 >= 0x10 && cmd1 <= 0x17)				// 2-byte codes
				{
					switch (cmd1)
					{
						case 0x10:	break;					// (note: EXT1 (0x10) is handled in "bExtended" code below)
						default:	break;					// (no other two-byte commands are currently defined)
					}
				}
				else // if (currByte >= 0x18 && currByte <= 0x1F)	// 3-byte codes
				{
					switch (cmd1)
					{
						case 0x18:	break;					//	P16:	this is supposedly the beginning of a three-byte command,
															//			which CEA-708C describes as "a further code space extension
															//			for 16-bit character sets." Does this mean we should inject
															//			a space or underline character here to denote "unknown character"?
						default:	break;
					}
				}
			}	// Code Set C0
			else if (cmd1 >= 0x20 && cmd1 <= 0x7F)		// Code Set G0
				AddCharacter (cmd1, NTV2_CC708CodeGroup_G0);
			else if (cmd1 >= 0x80 && cmd1 <= 0x9F)		// Code Set C1
			{
				if (cmd1 >= 0x80 && cmd1 <= 0x87)
					SetCurrentWindow (cmd1 & 0x07);	//	SetCurrentWindow command (no extra params)
				else if (cmd1 == 0x88)
					ClearWindows (pData[1]);	//	ClearWindows command - next byte is bitmap of windows to clear
				else if (cmd1 == 0x89)
					DisplayWindows (pData[1]);	//	DisplayWindows command - next byte is bitmap of windows to display
				else if (cmd1 == 0x8A)
					HideWindows (pData[1]);		//	HideWindows command - next byte is bitmap of windows to hide
				else if (cmd1 == 0x8B)
					ToggleWindows (pData[1]);	//	ToggleWindows command - next byte is bitmap of windows to toggle
				else if (cmd1 == 0x8C)
					DeleteWindows (pData[1]);	//	DeleteWindows command - next byte is bitmap of windows to delete
				else if (cmd1 == 0x8D)
					Delay (pData[1]);			//	Delay command - next byte is delay timeout (in tenths of seconds)
				else if (cmd1 == 0x8E)
					DelayCancel ();				//	DelayCancel command - no params
				else if (cmd1 == 0x8F)
					Reset ();					//	Reset command - no params
				else if (cmd1 == 0x90)
					SetPenAttributes (CC708PenAttr (pData[1], pData[2]));							//	SetPenAttributes cmd - next 2 bytes are params
				else if (cmd1 == 0x91)
					SetPenColor (CC708PenColor (pData[1], pData[2], pData[3]));						//	SetPenColor cmd - next 3 bytes are params
				else if (cmd1 == 0x92)
					SetPenLocation (CC708PenLocation (pData[1] & 0x0F, pData[2] & 0x3F));			//	SetPenLocation cmd - next 2 bytes are params
				else if (cmd1 == 0x97)
					SetWindowAttributes (CC708WindowAttr (pData[1], pData[2], pData[3], pData[4]));	//	SetWindowAttributes cmd - next 4 bytes are params
				else if (cmd1 >= 0x98 && cmd1 <= 0x9F)
					DefineWindow (cmd1 & 0x07, CC708WindowParms (pData[1], pData[2], pData[3], pData[4], pData[5], pData[6]));	//	DefineWindow cmd - next 6 bytes are params
				else	// ??? undefined command - ignore ???
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [???]");
			}	// Code Set C1
			else  // (currByte >= 0xA0 && currByte <= 0xFF)		// Code Set G1
				AddCharacter (cmd1, NTV2_CC708CodeGroup_G1);
		}	// if (!bExtended)
		else
		{
			//	Extended Code Set (the first byte was 'EXT1' (0x10)
			//	The specific command (and length) now depends on the second byte
			UByte cmd2 = pData [1];		//	Get 2nd byte

			if (/*cmd2 >= 0x00 && */ cmd2 <= 0x1f)		// C2 Code Set - the length depends on the 2nd byte
			{
				if (/*cmd2 >= 0x00 && */ cmd2 <= 0x07)		// 2-byte codes (EXT1 + cmd2)
				{
				}
				else if (cmd2 >= 0x08 && cmd2 <= 0x0f)		// 3-byte codes (EXT1 + cmd2 + 1 data byte)
				{
				}
				else if (cmd2 >= 0x10 && cmd2 <= 0x17)		// 4-byte codes (EXT1 + cmd2 + 2 data bytes)
				{
				}
				else /* (cmd2 >= 0x18 && cmd2 <= 0x1f) */	// 5-byte codes (EXT1 + cmd2 + 3 data bytes)
				{
				}
			}
			else if (cmd2 >= 0x20 && cmd2 <= 0x7f)		// G2 Code Set
			{
				AddCharacter (cmd2, NTV2_CC708CodeGroup_G2);
			}
			else if (cmd2 >= 0x80 && cmd2 <= 0x9f)		// C3 Code Set - the length depends on the 2nd byte
			{
				if		(cmd2 >= 0x80 && cmd2 <= 0x87)		// 6-byte codes (EXT1 + cmd2 + 4 data bytes)
				{
				}
				else if	(cmd2 >= 0x88 && cmd2 <= 0x8f)		// 7-byte codes (EXT1 + cmd2 + 5 data bytes)
				{
				}
				else /*	(cmd2 >= 0x90 && cmd2 <= 0x9f) */		// variable-length commands
				{
					//	The third byte contains the command header, which holds the "type" code and the length
					//	make sure there is a third byte...
					cmdLength   = (pData [2] & 0x1f) + 3;		// EXT1 + cmd2 + varLengthHdr + varLengthData
					//int cmdType = (pData [2] & 0xc0) >> 6;	// '00' = Beginning of Command (BOC), '01' = Continuation of Command (COC), '11' = End of Command (EOC)
				}
			}
			else /* (cmd2 >= 0xa0 && cmd2 <= 0xff) */		// G3 Code Set
				AddCharacter (cmd2, NTV2_CC708CodeGroup_G3);
		}	//	else bExtended
	}	// if (cmdLength > 0)

	return cmdLength;

}	//	Parse708Command


#define	WMAP(__w__)			((__w__) & 0x80 ? "7" : " ") << ((__w__) & 0x40 ? "6" : " ") << ((__w__) & 0x20 ? "5" : " ") << ((__w__) & 0x10 ? "4" : " ")	\
						<<  ((__w__) & 0x08 ? "3" : " ") << ((__w__) & 0x04 ? "2" : " ") << ((__w__) & 0x02 ? "1" : " ") << ((__w__) & 0x01 ? "0" : " ")


// DebugParseCommand()
//
//	Print useful info about one or more 708 Caption Data bytes, which start at pData[0].
//	Note that dataSize is NOT necessarily the size of the command we're parsing - it's only
//	an indication of how much valid data is in pData so we don't walk off the end of an array.
//	Returns the size of the command, or '0' if error in parsing.
//
size_t CNTV2Caption708Service::DebugParse708Command (const UByte * pData, const size_t dataSize) const
{
	//	Reality check...
	if (pData == NULL || dataSize == 0)
	{
		LOGMYERROR("bad params");
		return 0;
	}

	size_t	cmdLength	(GetCommandSize (pData, dataSize));
	if (cmdLength > 0)
	{
		// There are four standard "Code Groups" (CL, GL, CR, and GR), determined by ranges of the 8-bit data:
		//
		//		 Data Range  Code		Standard	Extended
		//					 Group		  Set		  Set
		//		0x00 - 0x1F:  CL		  C0		  C2
		//		0x20 - 0x7F:  GL		  G0		  G2
		//		0x80 - 0x9F:  CR		  C1		  C3
		//		0xA0 - 0xFF:  GR		  G1		  G3
		//
		// Each Code Group contains two "Code Sets", "Standard" and "Extended". The Extended Code Sets are accessed by first
		// transmitting an EXT1 byte (0x10). Without this byte, the Standard Code Sets are assumed.

		//	See if this is an EXT1 byte
		const UByte	cmd1		(pData [0]);	// shortcut
		const bool	bExtended	(cmd1 == 0x10);

		if (!bExtended)
		{
			//	Standard Code Set
			if (/*cmd1 >= 0x00 &&*/ cmd1 <= 0x1F)		// Code Set C0
			{
				//	This is further broken into 3 sections:
				//		0x00 - 0x0F are 1-byte codes
				//		0x10 - 0x17 are 2-byte codes
				//		0x18 - 0x1F are 3-byte codes
				string	which;

				if (/*cmd1 >= 0x00 &&*/ cmd1 <= 0x0F)			// 1-byte codes
				{
					switch (cmd1)
					{
						case 0x00:	which = "NUL";		break;
						case 0x01:	which = "???";		break;
						case 0x02:	which = "???";		break;
						case 0x03:	which = "ETX";		break;
						case 0x04:	which = "???";		break;
						case 0x05:	which = "???";		break;
						case 0x06:	which = "???";		break;
						case 0x07:	which = "???";		break;
						case 0x08:	which = "BS";		break;
						case 0x09:	which = "???";		break;
						case 0x0A:	which = "???";		break;
						case 0x0B:	which = "???";		break;
						case 0x0C:	which = "FF";		break;
						case 0x0D:	which = "CR";		break;
						case 0x0E:	which = "HCR";		break;
						case 0x0F:	which = "???";		break;
					}

					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C0 Data - [" << which << "]");
				}
				else if (cmd1 >= 0x10 && cmd1 <= 0x17)			// 2-byte codes
				{
					switch (cmd1)
					{
						case 0x10:	which = "EXT1";		break;		// (note: EXT1 (0x10) is handled in "bExtended" code below)
						case 0x11:	which = "???";		break;
						case 0x12:	which = "???";		break;
						case 0x13:	which = "???";		break;
						case 0x14:	which = "???";		break;
						case 0x15:	which = "???";		break;
						case 0x16:	which = "???";		break;
						case 0x17:	which = "???";		break;
					}

					UByte cmd2 = pData [1];		// get 2nd byte

					LOGMYINFO ("         0x" << UHEX2(cmd1) << " 0x" << UHEX2(cmd2) << ": C0 Data - [" << which << "]  2-byte command");
				}
				else	// if (currByte >= 0x18 && currByte <= 0x1F)
				{
					//	3-byte codes
					switch (cmd1)
					{
						case 0x18:	which = "P16";		break;
						case 0x19:	which = "???";		break;
						case 0x1A:	which = "???";		break;
						case 0x1B:	which = "???";		break;
						case 0x1C:	which = "???";		break;
						case 0x1D:	which = "???";		break;
						case 0x1E:	which = "???";		break;
						case 0x1F:	which = "???";		break;
					}

					UByte cmd2 = pData [1];		// get 2nd byte
					UByte cmd3 = pData [2];		// get 3rd byte

					LOGMYINFO ("         0x" << UHEX2(cmd1) << " 0x" << UHEX2(cmd2) << " 0x" << UHEX2(cmd3) << ": C0 Data - [" << which << "]  3-byte command");
				}
			}	// Code Set C0
			else if (cmd1 >= 0x20 && cmd1 <= 0x7F)		// Code Set G0
				LOGMYINFO ("         0x" << UHEX2(cmd1) << ": G0 Data - '" << cmd1 << "'");
			else if (cmd1 >= 0x80 && cmd1 <= 0x9F)		// Code Set C1
			{
				if (cmd1 >= 0x80 && cmd1 <= 0x87)
				{
					//	SetCurrentWindow command (no extra params)
					int windowID = cmd1 & 0x07;
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [CW" << windowID << "], Set Current Window = " << windowID);
				}
				else if (cmd1 == 0x88)
				{
					//	ClearWindows command - next byte is bitmap of windows to clear
					UByte wmap = pData [1];
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [CLW], ClearWindows 0x" << UHEX2(wmap) << " (" << WMAP (wmap) << ")");
				}
				else if (cmd1 == 0x89)
				{
					//	DisplayWindows command - next byte is bitmap of windows to display
					UByte wmap = pData [1];
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [DSW], DisplayWindows 0x" << UHEX2(wmap) << " (" << WMAP (wmap) << ")");
				}
				else if (cmd1 == 0x8A)
				{
					//	HideWindows command - next byte is bitmap of windows to hide
					UByte wmap = pData [1];
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [HDW], HideWindows 0x" << UHEX2(wmap) << " (" << WMAP (wmap) << ")");
				}
				else if (cmd1 == 0x8B)
				{
					//	ToggleWindows command - next byte is bitmap of windows to toggle
					UByte wmap = pData [1];
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [TGW], ToggleWindows 0x" << UHEX2(wmap) << " (" << WMAP (wmap) << ")");
				}
				else if (cmd1 == 0x8C)
				{
					//	DeleteWindows command - next byte is bitmap of windows to delete
					UByte wmap = pData [1];
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [DLW], DeleteWindows 0x" << UHEX2(wmap) << " (" << WMAP (wmap) << ")");
				}
				else if (cmd1 == 0x8D)
				{
					//	Delay command - next byte is delay timeout (in tenths of seconds)
					UByte delay = pData [1];
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [DLY], Delay = " << delay << "/10 seconds");
				}
				else if (cmd1 == 0x8E)
				{
					//	DelayCancel command - no params
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [DLC], DelayCancel");
				}
				else if (cmd1 == 0x8F)
				{
					//	Reset command - no params
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [RST], Reset");
				}
				else if (cmd1 == 0x90)
				{
					//	SetPenAttributes command - next 2 bytes are params
					UByte parm1 = pData [1];
					UByte parm2 = pData [2];
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [SPA], SetPenAttributes" << endl
													<< "                                  text tag   = " << ((parm1 & 0xF0) >> 4) << endl
													<< "                                  offset     = " << ((parm1 & 0x0C) >> 2) << endl
													<< "                                  pen size   = " << ((parm1 & 0x03)     ) << endl
													<< "                                  italics    = " << ((parm2 & 0x80) >> 7) << endl
													<< "                                  underline  = " << ((parm2 & 0x40) >> 6) << endl
													<< "                                  edge type  = " << ((parm2 & 0x38) >> 3) << endl
													<< "                                  font style = " <<  (parm2 & 0x07));
				}
				else if (cmd1 == 0x91)
				{
					//	SetPenColor command - next 3 bytes are params
					UByte parm1 = pData [1];
					UByte parm2 = pData [2];
					UByte parm3 = pData [3];
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [SPC], SetPenColor" << endl
						<< "                                  fg opacity = " << ((parm1 & 0xC0) >> 6) << endl
						<< "                                  fg r/g/b   = " << ((parm1 & 0x30) >> 4) << "/" << ((parm1 & 0x0C) >> 2) << "/" << (parm1 & 0x03) << endl
						<< "                                  bg opacity = " << ((parm2 & 0xC0) >> 6) << endl
						<< "                                  bg r/g/b   = " << ((parm2 & 0x30) >> 4) << "/" << ((parm2 & 0x0C) >> 2) << "/" << (parm2 & 0x03) << endl
						<< "                                  edge r/g/b = " << ((parm3 & 0x30) >> 4) << "/" << ((parm3 & 0x0C) >> 2) << "/" << (parm3 & 0x03));
				}
				else if (cmd1 == 0x92)
				{
					//	SetPenLocation command - next 2 bytes are params
					UByte parm1 = pData [1];
					UByte parm2 = pData [2];
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [SPL], SetPenLocation, row = " << (parm1 & 0x0F) << ", column = " << (parm2 & 0x3F));
				}
				else if (cmd1 == 0x97)
				{
					//	SetWindowAttributes command - next 4 bytes are params
					UByte parm1 = pData [1];
					UByte parm2 = pData [2];
					UByte parm3 = pData [3];
					UByte parm4 = pData [4];
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [SWA], SetWindowAttributes" << endl
										<< "                                  fill opacity = " <<  ((parm1 & 0xC0) >> 6) << endl
										<< "                                  fill r/g/b   = " <<  ((parm1 & 0x30) >> 4) << "/" << ((parm1 & 0x0C) >> 2) << "/" << (parm1 & 0x03) << endl
										<< "                                  border type  = " << (((parm3 & 0x80) >> 5) + ((parm2 & 0xC0) >> 6)) << endl
										<< "                                  border r/g/b = " <<  ((parm2 & 0x30) >> 4) << "/" << ((parm2 & 0x0C) >> 2) << "/" << (parm2 & 0x03) << endl
										<< "                                  word wrap    = " <<  ((parm3 & 0x40) >> 6) << endl
										<< "                                  print dir    = " <<  ((parm3 & 0x30) >> 4) << endl
										<< "                                  scroll dir   = " <<  ((parm3 & 0x0C) >> 2) << endl
										<< "                                  justify      = " <<  ((parm3 & 0x03)     ) << endl
										<< "                                  effect speed = " <<  ((parm4 & 0xF0) >> 4) << endl
										<< "                                  effect dir   = " <<  ((parm4 & 0x0C) >> 2) << endl
										<< "                                  disp effect  = " <<  ((parm4 & 0x03)     ));
				}
				else if (cmd1 >= 0x98 && cmd1 <= 0x9F)
				{
					//	DefineWindow command - next 6 bytes are params
					int win = cmd1 & 0x07;
					UByte parm1 = pData [1];
					UByte parm2 = pData [2];
					UByte parm3 = pData [3];
					UByte parm4 = pData [4];
					UByte parm5 = pData [5];
					UByte parm6 = pData [6];
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [DF" << win << "], DefineWindow" << win << endl
										<< "                                  visible   = " << ((parm1 & 0x20) >> 5)  << endl
										<< "                                  row lock  = " << ((parm1 & 0x10) >> 4)  << endl
										<< "                                  col lock  = " << ((parm1 & 0x08) >> 3)  << endl
										<< "                                  priority  = " << ((parm1 & 0x07)     )  << endl
										<< "                                  rel pos   = " << ((parm2 & 0x80) >> 7)  << endl
										<< "                                  anchor V  = " << ((parm2 & 0x7F)     )  << endl
										<< "                                  anchor H  = " << ((parm3 & 0xFF)     )  << endl
										<< "                                  anchor pt = " << ((parm4 & 0xF0) >> 4)  << endl
										<< "                                  row count = " <<  (parm4 & 0x0F)        << " (+1)" << endl
										<< "                                  col count = " <<  (parm5 & 0x3F)        << " (+1)" << endl
										<< "                                  wnd style = " << ((parm6 & 0x38) >> 3)  << endl
										<< "                                  pen style = " <<  (parm6 & 0x07));
				}
				else	// ??? undefined ???
					LOGMYINFO ("         0x" << UHEX2(cmd1) << ": C1 Data - [???]");
			}	// Code Set C1
			else  // (currByte >= 0xA0 && currByte <= 0xFF)		// Code Set G1 (consists of non-ASCII characters we can't print on the Console...)
			{
				LOGMYINFO ("         0x" << UHEX2(cmd1) << ": G1 Data");
			}	// Code Set G1

		}	// if (!bExtended)
		else
		{
			//	Extended Code Set (the first byte was 'EXT1' (0x10)
			//	The specific command (and length) now depends on the second byte
			const string	which	("EXT1");
			string			which2	("???");
			const UByte		cmd2	(pData [1]);		// get 2nd byte

			if (/*cmd2 >= 0x00 && */ cmd2 <= 0x1f)		// C2 Code Set - the length depends on the 2nd byte
			{
				if (/*cmd2 >= 0x00 && */ cmd2 <= 0x07)		// 2-byte codes (EXT1 + cmd2)
					LOGMYINFO ("         0x" << UHEX2(cmd1) << " 0x" << UHEX2(cmd2) << ": C2 Data - [" << which << "], ???  (2-byte code)");
				else if (cmd2 >= 0x08 && cmd2 <= 0x0f)		// 3-byte codes (EXT1 + cmd2 + 1 data byte)
					LOGMYINFO ("         0x" << UHEX2(cmd1) << " 0x" << UHEX2(cmd2) << " 0x" << UHEX2(pData[2]) << ": C2 Data - [" << which << "], ??? ???  (3-byte code)");
				else if (cmd2 >= 0x10 && cmd2 <= 0x17)		// 4-byte codes (EXT1 + cmd2 + 2 data bytes)
					LOGMYINFO ("         0x" << UHEX2(cmd1) << " 0x" << UHEX2(cmd2) << " 0x" << UHEX2(pData[2]) << " 0x" << UHEX2(pData[3]) << ": C2 Data - [" << which << "], ??? ??? ??? (4-byte code)");
				else /* (cmd2 >= 0x18 && cmd2 <= 0x1f) */	// 5-byte codes (EXT1 + cmd2 + 3 data bytes)
					LOGMYINFO ("         0x" << UHEX2(cmd1) << " 0x" << UHEX2(cmd2) << " 0x" << UHEX2(pData[2]) << " 0x" << UHEX2(pData[3]) << " 0x" << UHEX2(pData[4]) << ": C2 Data - [" << which << "], ??? ??? ??? ??? (5-byte code)");
			}
			else if (cmd2 >= 0x20 && cmd2 <= 0x7f)		// G2 Code Set - print the ones we know about...
			{
				switch (cmd2)
				{
					case 0x20:	which2 = "[TSP]";	break;
					case 0x21:	which2 = "[NBTSP]";	break;
					case 0x25:	which2 = "...";		break;
					case 0x2a:	which2 = "S";		break;
					case 0x2c:	which2 = "OE";		break;
					case 0x30:	which2 = "[Block]";	break;
					case 0x31:	which2 = "`";		break;
					case 0x32:	which2 = "'";		break;
					case 0x33:	which2 = "\"";		break;
					case 0x34:	which2 = "\"";		break;
					case 0x35:	which2 = ".";		break;
					case 0x39:	which2 = "TM";		break;
					case 0x3a:	which2 = "s";		break;
					case 0x3c:	which2 = "oe";		break;
					case 0x3d:	which2 = "SM";		break;
					case 0x3f:	which2 = "Y";		break;

					case 0x76:	which2 = "1/8";		break;
					case 0x77:	which2 = "3/8";		break;
					case 0x78:	which2 = "5/8";		break;
					case 0x79:	which2 = "7/8";		break;
					case 0x7a:	which2 = "|";		break;
					case 0x7b:	which2 = "-|";		break;
					case 0x7c:	which2 = "|_";		break;
					case 0x7d:	which2 = "_";		break;
					case 0x7e:	which2 = "_|";		break;
					case 0x7f:	which2 = "|-";		break;
				}
				LOGMYINFO ("         0x" << UHEX2(cmd1) << " 0x" << UHEX2(cmd2) << ": G2 Data - [" << which << "], " << which2 << "  (2-byte code)");
			}
			else if (cmd2 >= 0x80 && cmd2 <= 0x9f)		// C3 Code Set - the length depends on the 2nd byte
			{
				if      (cmd2 >= 0x80 && cmd2 <= 0x87)		// 6-byte codes (EXT1 + cmd2 + 4 data bytes)
					LOGMYINFO ("         0x" << UHEX2(cmd1) << " 0x" << UHEX2(cmd2) << " 0x" << UHEX2(pData[2]) << " 0x" << UHEX2(pData[3]) << " 0x" << UHEX2(pData[4])
										<< " 0x" << UHEX2(pData[5]) << ": C3 Data - [" << which << "], ??? ??? ??? ??? ??? (6-byte code)");
				else if (cmd2 >= 0x88 && cmd2 <= 0x8f)		// 7-byte codes (EXT1 + cmd2 + 5 data bytes)
					LOGMYINFO ("         0x" << UHEX2(cmd1) << " 0x" << UHEX2(cmd2) << " 0x" << UHEX2(pData[2]) << " 0x" << UHEX2(pData[3]) << " 0x" << UHEX2(pData[4])
										<< " 0x" << UHEX2(pData[5]) << " 0x" << UHEX2(pData[6]) << ": C2 Data - [" << which << "], ??? ??? ??? ??? ??? ??? (7-byte code)");
				else /* (cmd2 >= 0x90 && cmd2 <= 0x9f) */		// variable-length commands
				{
					// (what a nightmare..._
					// the third byte contains the command header, which holds the "type" code and the length
					// make sure there is a third byte...
					cmdLength   = (pData [2] & 0x1f) + 3;		// EXT1 + cmd2 + varLengthHdr + varLengthData
					int cmdType = (pData [2] & 0xc0) >> 6;		// '00' = Beginning of Command (BOC), '01' = Continuation of Command (COC), '11' = End of Command (EOC)

					LOGMYINFO ("         0x" << UHEX2(cmd1) << " 0x" << UHEX2(cmd2) << " 0x" << UHEX2(pData[2]) << ": C3 Data - [" << which
										<< "], ??? ??? variable length (type " << cmdType << ": " << (cmdLength - 3) << " data bytes) code");
				}
			}
			else /* (cmd2 >= 0xa0 && cmd2 <= 0xff) */		// G3 Code Set - print the one(s) we know about...
			{
				switch (cmd2)
				{
					case 0xa0:	which2 = "CC";		break;
				}

				LOGMYINFO ("         0x" << UHEX2(cmd1) << " 0x" << UHEX2(cmd2) << ": G3 Data - [" << which << "], " << which2 << "  (2-byte code)");
			}
		}
	}	// if (cmdLength > 0)

	return cmdLength;

}	//	DebugParse708Command


//--------------- 708 Basic Commands --------------------------------------------

// SetCurrentWindow()
//	Specify current window ID - see CEA-708C Section 8.10.5.1
//
bool CNTV2Caption708Service::SetCurrentWindow (const int inWindowID)
{
	//LOGMYINFO ("         0x%02x: [CW%1d], Set Current Window = %1d\n", (0x80 + windowID), windowID, windowID);
	if (inWindowID < NTV2_CC708WindowIDMin || inWindowID > NTV2_CC708WindowIDMax)
		return false;
	mCurrentWindow = inWindowID;
	return true;
}


// ClearWindows()
//	Clears text from a set of windows - see CEA-708C Section 8.10.5.3
//
bool CNTV2Caption708Service::ClearWindows (const UByte inWindowMap)
{
	//LOGMYINFO ("         0x88: [CLW], ClearWindows 0x%02x (%c%c%c%c%c%c%c%c)\n", inWindowMap,
	//							(inWindowMap & 0x80 ? '7' : ' '), (inWindowMap & 0x40 ? '6' : ' '), (inWindowMap & 0x20 ? '5' : ' '), (inWindowMap & 0x10 ? '4' : ' '),
	//							(inWindowMap & 0x08 ? '3' : ' '), (inWindowMap & 0x04 ? '2' : ' '), (inWindowMap & 0x02 ? '1' : ' '), (inWindowMap & 0x01 ? '0' : ' ') );
	for (int windowID (0);  windowID < NTV2_CC708NumWindows;  windowID++)
		if (inWindowMap & (1 << windowID))
			mWindowArray[windowID].EraseWindowText ();
	return true;
}


// DeleteWindows()
//	Deletes window definitions for a set of windows - see CEA-708C Section 8.10.5.4
//
bool CNTV2Caption708Service::DeleteWindows (const UByte inWindowMap)
{
	//LOGMYINFO ("         0x8c: [DLW], DeleteWindows 0x%02x (%c%c%c%c%c%c%c%c)\n", inWindowMap,
	//							(inWindowMap & 0x80 ? '7' : ' '), (inWindowMap & 0x40 ? '6' : ' '), (inWindowMap & 0x20 ? '5' : ' '), (inWindowMap & 0x10 ? '4' : ' '),
	//							(inWindowMap & 0x08 ? '3' : ' '), (inWindowMap & 0x04 ? '2' : ' '), (inWindowMap & 0x02 ? '1' : ' '), (inWindowMap & 0x01 ? '0' : ' ') );
	for (int windowID (0);  windowID < NTV2_CC708NumWindows;  windowID++)
		if (inWindowMap & (1 << windowID))
			mWindowArray[windowID].InitWindow ();
	return true;
}


// DisplayWindows()
//	Causes a set of windows to become visible - see CEA-708C Section 8.10.5.5
//
bool CNTV2Caption708Service::DisplayWindows (const UByte inWindowMap)
{
	//LOGMYINFO ("         0x89: [DSW], DisplayWindows 0x%02x (%c%c%c%c%c%c%c%c)\n", inWindowMap,
	//							(inWindowMap & 0x80 ? '7' : ' '), (inWindowMap & 0x40 ? '6' : ' '), (inWindowMap & 0x20 ? '5' : ' '), (inWindowMap & 0x10 ? '4' : ' '),
	//							(inWindowMap & 0x08 ? '3' : ' '), (inWindowMap & 0x04 ? '2' : ' '), (inWindowMap & 0x02 ? '1' : ' '), (inWindowMap & 0x01 ? '0' : ' '));
	for (int windowID (0);  windowID < NTV2_CC708NumWindows;  windowID++)
		if (inWindowMap & (1 << windowID))
			mWindowArray[windowID].SetVisible (true);
	return true;
}


// HideWindows()
//	Causes a set of windows to become invisible - see CEA-708C Section 8.10.5.6
//
bool CNTV2Caption708Service::HideWindows (const UByte inWindowMap)
{
	//LOGMYINFO ("         0x8a: [HDW], HideWindows 0x%02x (%c%c%c%c%c%c%c%c)\n", inWindowMap,
	//							(inWindowMap & 0x80 ? '7' : ' '), (inWindowMap & 0x40 ? '6' : ' '), (inWindowMap & 0x20 ? '5' : ' '), (inWindowMap & 0x10 ? '4' : ' '),
	//							(inWindowMap & 0x08 ? '3' : ' '), (inWindowMap & 0x04 ? '2' : ' '), (inWindowMap & 0x02 ? '1' : ' '), (inWindowMap & 0x01 ? '0' : ' '));
	for (int windowID (0);  windowID < NTV2_CC708NumWindows;  windowID++)
		if (inWindowMap & (1 << windowID))
			mWindowArray[windowID].SetVisible (false);
	return true;
}


// ToggleWindows()
//	Toggles Display/Hide status of a set of windows - see CEA-708C Section 8.10.5.7
//
bool CNTV2Caption708Service::ToggleWindows (const UByte inWindowMap)
{
	//LOGMYINFO ("         0x8b: [TGW], ToggleWindows 0x%02x (%c%c%c%c%c%c%c%c)\n", inWindowMap,
	//						(inWindowMap & 0x80 ? '7' : ' '), (inWindowMap & 0x40 ? '6' : ' '), (inWindowMap & 0x20 ? '5' : ' '), (inWindowMap & 0x10 ? '4' : ' '),
	//						(inWindowMap & 0x08 ? '3' : ' '), (inWindowMap & 0x04 ? '2' : ' '), (inWindowMap & 0x02 ? '1' : ' '), (inWindowMap & 0x01 ? '0' : ' '));
	for (int windowID (0);  windowID < NTV2_CC708NumWindows;  windowID++)
		if (inWindowMap & (1 << windowID))
			mWindowArray[windowID].SetVisible (!mWindowArray[windowID].GetVisible ());
	return true;
}




// DefineWindow()
//	Create window and set initial parameters - see CEA-708C Section 8.10.5.2
//
bool CNTV2Caption708Service::DefineWindow (const int inWindowID, const CC708WindowParms & inParameters)
{
	//LOGMYINFO ("         0x%02x: C1 Data - [DF%1d], DefineWindow%1d\n", (0x98 + windowID), win, win);
	if (inWindowID < NTV2_CC708WindowIDMin  ||  inWindowID > NTV2_CC708WindowIDMax)
		return false;

	SetCurrentWindow (inWindowID);
	mWindowArray[mCurrentWindow].DefineWindow (inParameters);
	mWindowArray[mCurrentWindow].SetWindowID (inWindowID);
	//mWindowArray[mCurrentWindow].DebugPrintWindowParameters ();
	return true;
}



// SetWindowAttributes()
//	Defines the window styles for the current window - see CEA-708C Section 8.10.5.8
//
bool CNTV2Caption708Service::SetWindowAttributes (const CC708WindowAttr & inAttr)
{
	//LOGMYINFO ("         [SWA], SetWindowAttributes - window " << mCurrentWindow);
	if (mCurrentWindow < NTV2_CC708WindowIDMin  ||  mCurrentWindow > NTV2_CC708WindowIDMax)
		return false;

	mWindowArray[mCurrentWindow].SetWindowAttributes (inAttr);
	//mWindowArray[mCurrentWindow].DebugPrintWindowAttributes ();
	return true;
}



// SetPenAttributes()
//	Assign pen style attributes for the current window - see CEA-708C Section 8.10.5.9
//
void CNTV2Caption708Service::SetPenAttributes (const CC708PenAttr & inAttr)
{
	//LOGMYINFO ("         0x90: [SPA], SetPenAttributes - window " << mCurrentWindow);
	if (mCurrentWindow >= NTV2_CC708WindowIDMin  &&  mCurrentWindow <= NTV2_CC708WindowIDMax)
	{
		mWindowArray[mCurrentWindow].SetPenAttributes (inAttr);
		//mWindowArray[mCurrentWindow].DebugPrintPenAttributes ();
	}
}


// SetPenLocation()
//	Specifies the pen cursor location within a window - see CEA-708C Section 8.10.5.11
//
void CNTV2Caption708Service::SetPenLocation (const CC708PenLocation & inLoc)
{
	//LOGMYINFO ("         0x92: [SPL], SetPenLocation, row = %d, column = %d\n", pPenLoc->row, pPenLoc->column);
	if (mCurrentWindow >= NTV2_CC708WindowIDMin  &&  mCurrentWindow <= NTV2_CC708WindowIDMax)
	{
		mWindowArray[mCurrentWindow].SetPenLocation (inLoc);
		//mWindowArray[mCurrentWindow].DebugPrintPenLocation ();
	}
}



// SetPenColor()
//	Specifies the pen color within a window. See CEA-708-D Section 8.10.5.10.
//
void CNTV2Caption708Service::SetPenColor (const CC708PenColor & inColor)
{
	//LOGMYINFO ("         0x91: [SPC], SetPenColor");
	if (mCurrentWindow >= NTV2_CC708WindowIDMin  &&  mCurrentWindow <= NTV2_CC708WindowIDMax)
	{
		mWindowArray[mCurrentWindow].SetPenColor (inColor);
		//mWindowArray[mCurrentWindow].DebugPrintPenColor ();
	}
}



// Delay()
//	Delays service data interpretation - see CEA-708C Section 8.10.5.12
//
void CNTV2Caption708Service::Delay (const int inTenthsSec)
{
	(void) inTenthsSec;
	//	LOGMYINFO ("         0x8d: [DLY], Delay " << tenthsSec);
	// TBD...
}


// DelayCancel()
//	Cancels an Active Delay Command - see CEA-708C Section 8.10.5.13
//
void CNTV2Caption708Service::DelayCancel (void)
{
	//LOGMYINFO ("         0x8e: [DLC], Delay Cancel");
	// TBD...
}


// Reset()
//	Resets the Caption Channel Service - see CEA-708C Section 8.10.5.14
//
void CNTV2Caption708Service::Reset (void)
{
	//LOGMYINFO ("         0x8f: [RST], Reset");
	InitService (mServiceInfo.captionSvcNumber);
}


// AddCharacter()
//
void CNTV2Caption708Service::AddCharacter (const UByte inChar, const CC708CodeGroup inCodeGroup)
{
	//	LOGMYINFO ("         0x" <<  UHEX2(theChar) << ": Character");
	if (mCurrentWindow >= NTV2_CC708WindowIDMin  &&  mCurrentWindow <= NTV2_CC708WindowIDMax)
		mWindowArray [mCurrentWindow].AddCharacter (inChar, inCodeGroup);
}


// DoETX()
//
void CNTV2Caption708Service::DoETX (void)
{
	//LOGMYINFO ("         0x03: [ETX]");
	if (mCurrentWindow >= NTV2_CC708WindowIDMin  &&  mCurrentWindow <= NTV2_CC708WindowIDMax)
		mWindowArray [mCurrentWindow].DoETX ();
}


// DoBS()
//
void CNTV2Caption708Service::DoBS (void)
{
	//LOGMYINFO ("         0x08: [BS]");
	if (mCurrentWindow >= NTV2_CC708WindowIDMin  &&  mCurrentWindow <= NTV2_CC708WindowIDMax)
		mWindowArray [mCurrentWindow].DoBS ();
}


// DoFF()
//
void CNTV2Caption708Service::DoFF (void)
{
	//LOGMYINFO ("         0x0c: [FF]");
	if (mCurrentWindow >= NTV2_CC708WindowIDMin  &&  mCurrentWindow <= NTV2_CC708WindowIDMax)
		mWindowArray [mCurrentWindow].DoFF ();
}


// DoCR()
//
void CNTV2Caption708Service::DoCR (void)
{
	//LOGMYINFO ("         0x0d: [CR]");
	if (mCurrentWindow >= NTV2_CC708WindowIDMin  &&  mCurrentWindow <= NTV2_CC708WindowIDMax)
		mWindowArray [mCurrentWindow].DoCR ();
}


// DoHCR()
//
void CNTV2Caption708Service::DoHCR (void)
{
	//LOGMYINFO ("         0x0e: [HCR]");
	if (mCurrentWindow >= NTV2_CC708WindowIDMin  &&  mCurrentWindow <= NTV2_CC708WindowIDMax)
		mWindowArray[mCurrentWindow].DoHCR ();
}


//---------------------------------------------------------------------------------------------------

// ParseInputServiceBlockToLocalQueue()
//		Parse an input Service Block into elementary commands and place them on our local queue as independent Service Blocks
//		pData points to 1st Data Word of Service Block (NOT the Service Block header!)
//
bool CNTV2Caption708Service::ParseInputServiceBlockToLocalQueue (const UByte * pData, const size_t inBlockSize)
{
	bool bErr = false;

	//	Make template for Service Block Header (we'll fill in the block size later)...
	bool  bExtendedHdr = mServiceInfo.captionSvcNumber > 6;

	//	For each command in the input Service Block, we're going to build an independent
	//	"single command" output Service Block and add it to our Service Block queue. While
	//	this is arguably an inefficient extra step if we're only decoding, it allows us to
	//	build a queue of self-contained elementary commands which can also be used to re-pack
	//	the output for translation to different frame rates.
	UByte	svcBlock [NTV2_CC708_MaxServiceBlockSize + 2];
	size_t	firstDataIndex;
	if (!bExtendedHdr)
	{
		//	Normal service blocks have a 1-byte header, followed by data starting in the 2nd byte...
		svcBlock [0] = (mServiceInfo.captionSvcNumber & 0x07) << 5;	//	Service number, but no block size (yet)
		firstDataIndex = 1;
	}
	else
	{
		//	Extended service blocks have a 2-byte header, followed by data starting in the 3rd byte...
		svcBlock [0] = 0xE0;										//	Service number = "7", but no block size (yet)
		svcBlock [1] = mServiceInfo.captionSvcNumber & 0x3F;		//	The real ("extended") service number
		firstDataIndex = 2;
	}

	//	Iterate through all commands in the input Service Block...
	size_t	index	(0);	//	Index into INPUT data (note: assuming first byte is Service Block data, not header!)

	while (!bErr && (index < inBlockSize))
	{
		const size_t	commandSize	(GetCommandSize (&pData [index], inBlockSize - index));

		//	Now build a new independent Service Block that contains only this command...
		//	(check for legal size and make sure the input buffer has all of the data we need)
		if (commandSize > 0  &&  ((index + commandSize) <= inBlockSize))
		{
			//	Fill in the Service Block data size...
			svcBlock[0] = (svcBlock[0] & 0xe0) + (commandSize & 0x1f);

			//	Copy the data bytes into our local Service Block, starting after the Service Block Header...
			size_t	ndx	(0);
			for (ndx = 0;  ndx < commandSize;  ndx++)
				svcBlock [firstDataIndex + ndx] = pData [index++];

			//	For safety's sake (?) clear the remaining data bytes...
			for (;  ndx < NTV2_CC708_MaxServiceBlockSize;  ndx++)
				svcBlock [firstDataIndex + ndx] = 0;

			//	Add our local Service Block to the Service Block Queue...
			if (!bErr)
				mServiceBlockQueue.PushServiceBlock (svcBlock, firstDataIndex + commandSize);
		}		
		else
			bErr = true;	//	If there's something wrong with the data, ignore the rest of the Service Block

		//	Continue as long as there are bytes in the input Service Block (or we run into an error)

	}	//	while (!bErr && (index < inBlockSize))

	return !bErr;

}	//	ParseInputServiceBlockToLocalQueue



// PeekNextServiceBlockInfo()
//
//	Returns the Block Size, Data Size, and Service Number of the next Service Block on the queue (returns false if empty).
//	This is a "peek" operation - the service block is left on the queue.
//
bool CNTV2Caption708Service::PeekNextServiceBlockInfo (size_t & outBlockSize, size_t & outDataSize, int & outServiceNum, bool & outIsExtended) const
{
	return mServiceBlockQueue.PeekNextServiceBlockInfo (outBlockSize, outDataSize, outServiceNum, outIsExtended);
}



// PopServiceBlock()
//
//	Copies the next Service Block from the queue to the designated pointer (also "pops" queue element).
//	This method copies the entire Service Block - to copy only the data, call PopServiceBlockData().
//	Returns the size of the copied data.
//
size_t CNTV2Caption708Service::PopServiceBlock (std::vector<UByte> & outData)
{
	return mServiceBlockQueue.PopServiceBlock (outData);
}

size_t CNTV2Caption708Service::PopServiceBlock (UByte *pData)
{
	return mServiceBlockQueue.PopServiceBlock (pData);
}



// PopServiceBlockData()
//
//	Copies the next Service Block (data only) from the queue to the designated pointer (also "pops" queue element).
//	This method only copies the Service Block payload - to copy the entire Service Block, call PopServiceBlock().
//	Returns the size of the copied data.
//
size_t CNTV2Caption708Service::PopServiceBlockData (std::vector<UByte> & outData)
{
	return mServiceBlockQueue.PopServiceBlockData (outData);
}

size_t CNTV2Caption708Service::PopServiceBlockData (UByte *pData)
{
	return mServiceBlockQueue.PopServiceBlockData (pData);
}




//-------- Debug --------

NTV2CaptionLogMask CNTV2Caption708Service::SetLogMask (const NTV2CaptionLogMask inLogMask)
{
	CNTV2CaptionLogConfig::SetLogMask (inLogMask);
	for (int i (0);  i < NTV2_CC708NumWindows;  i++)
		mWindowArray[i].SetLogMask (inLogMask);
	return mServiceBlockQueue.SetLogMask (inLogMask);
}
