/**
	@file		ntv2captionencoder708.cpp
	@brief		Implementation of the CNTV2CaptionEncoder708 class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/



/*
	This module contains a pile of utility methods for building CEA-708 ("DTVCC")
	closed-captioning commands. The commands are built in a local buffer, and
	commands may be concatenated by tracking the "index" into the buffer. Once the
	command(s) have been built, the local buffer may be accessed and/or copied to
	retrieve the results.

	There are two sets of methods in this class. The first set deals with
	relatively primitive 708 commands, and builds the results in mPacketData. The
	results can be retrieved using GetCaptionChannelPacket(), and/or left in place
	for use by MakeSMPTE334AncPacket().

	The second set of methods deals with building SMPTE 334 Ancillary Packets,
	and uses the mAnc334Data buffer. The main user method is MakeSMPTE334AncPacket(),
	which assumes that a valid Caption Channel Packet has already been built in
	mPacketData, copies it into a full SMPTE-334 Ancillary Packet, and returns a pointer
	to the mAnc334Data buffer.

 */

#include "ntv2captionencoder708.h"
#include "ntv2utils.h"
#include "ntv2smpteancdata.h"
#include "ajabase/system/debug.h"
#include <sstream>
#include <string.h>	//	for memset

using namespace std;


/////////////////////////////////////////////////////////////////////////////
// CaptionEncoder708 definition
/////////////////////////////////////////////////////////////////////////////

#define	LOGMYERROR(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Encode, AJA_DebugSeverity_Error,	GetLogLabel() << ":  " << AJAFUNC << ":  " << __xpr__)
#define	LOGMYWARN(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Encode, AJA_DebugSeverity_Warning,GetLogLabel() << ":  " << AJAFUNC << ":  " << __xpr__)
#define	LOGMYNOTE(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Encode, AJA_DebugSeverity_Info,	GetLogLabel() << ":  " << AJAFUNC << ":  " << __xpr__)
#define	LOGMYDEBUG(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Encode, AJA_DebugSeverity_Debug,	GetLogLabel() << ":  " << AJAFUNC << ":  " << __xpr__)


static uint32_t	gInstanceTally(0);


/////////////////////////////////////////////////////////////////////////////


bool CNTV2CaptionEncoder708::Create (CNTV2CaptionEncoder708Ptr & outEncoder)
{
	outEncoder = NULL;

	try
	{
		outEncoder = new CNTV2CaptionEncoder708;
	}
	catch (const std::bad_alloc &)
	{
	}
	return outEncoder;

}	//	Create


// Constructor
CNTV2CaptionEncoder708::CNTV2CaptionEncoder708 (void)
	:	mPacketDataSize			(0),
		mPacketSequenceNum		(0),
		mAnc334Size				(0),
		mCDPSequenceNum			(0),
		mNumServiceInfoPerCDP	(NTV2_CC708MaxCDPServices),
		mFlip608Characters		(false)
{
	AJAAtomic::Increment(&gInstanceTally);
	ostringstream oss;	oss << "CaptionEncoder708-" << gInstanceTally;
	SetLogLabel(oss.str());

	::memset (&mPacketData, 0, sizeof (mPacketData));
	::memset (&mAnc334Data, 0, sizeof (mAnc334Data));
	m608CaptionData.f1_char1 = m608CaptionData.f1_char2 = m608CaptionData.f2_char1 = m608CaptionData.f2_char2 = 0x00;
	m608CaptionData.bGotField1Data = m608CaptionData.bGotField2Data = false;
	//mNumServiceInfoPerCDP = 2;		//	For debugging:	Throttle the number of service_info thingies sent out each frame (CDP)
	
	Reset();

}	//	constructor

CNTV2CaptionEncoder708::CNTV2CaptionEncoder708 (const CNTV2CaptionEncoder708 & inEncoderToCopy)
{
	NTV2_ASSERT(false);
}

CNTV2CaptionEncoder708::~CNTV2CaptionEncoder708 ()
{
}	//	destructor



// Reset()
//
//	This can be called to "flush" the system of any in-progress data.
//	Note: this is NOT guaranteed to be thread-safe.
//
void CNTV2CaptionEncoder708::Reset (void)
{
	Clear608CaptionData ();
	InitCaptionChannelPacket ();

	mServiceInfo.InitAllServiceInfo ();

}	//	Reset


//******************************************************************************************************
//
//		These methods are used to build (relatively) primitive CEA-708 commands in the mPacketData buffer.
//	While this is called the "Caption Channel Packet buffer", it can also be (and is) used to build
//	lower-level constructs, such as 708 Service Blocks.
//
//******************************************************************************************************

// InitCaptionChannelPacket
//		Clear the Caption Channel Packet buffer and size count	
void CNTV2CaptionEncoder708::InitCaptionChannelPacket (void)
{
	::memset (mPacketData, 0, sizeof (mPacketData));
	mPacketDataSize = 0;

}	//	InitCaptionChannelPacket


// SetCaptionChannelPacket
//		Call this if you have already formated a Caption Channel Packet externally and want to load it into
//	the mPacketData buffer so it can be turned into a SMPTE-334 Ancillary packet. This method COPIES the source data.
bool CNTV2CaptionEncoder708::SetCaptionChannelPacket (const UBytePtr pInData, const size_t inNumBytes)
{
	InitCaptionChannelPacket ();	//	Clear old stuff...

	if (!pInData  ||  !inNumBytes  ||  (inNumBytes >= NTV2_CC708MaxPktSize))
		return false;

	::memcpy (mPacketData, pInData, inNumBytes);
	mPacketDataSize = inNumBytes;
	return true;

}	//	SetCaptionChannelPacket


// SetCaptionChannelPacketSize
//		Sets the current Caption Channel Packet size (in bytes). Returns 'false' if error.
//		This is an external method because caption channel packets are typically NOT built
//		"in order", i.e. the headers are put on AFTER the data and commands.
bool CNTV2CaptionEncoder708::SetCaptionChannelPacketSize (const size_t packetSize)
{
	mPacketDataSize = packetSize;
	return true;
}


// MakeCaptionChannelPacketHeader
//		Insert a Caption Channel Packet Header byte into the packet buffer at <index>
bool CNTV2CaptionEncoder708::MakeCaptionChannelPacketHeader (const size_t index, size_t packetSize, size_t & outNewIndex)
{
	bool			bResult		(true);
	const size_t	dataSize	(1);		// Caption Channel Packet headers are always exactly one byte

	//	Sanity check: the largest packet size we can support is 128 bytes (127 data + 1 header)
	//	If larger, we will truncate the size and go ahead and build it, but report an error.
	if (packetSize > 128)
	{
		LOGMYERROR("packetSize " << packetSize << " out of range (exceeds 128)");
		packetSize = 128;
		bResult = false;
	}

	//	The packet size (in bytes) translates to a "sizeID" which is 1/2 the actual size
	//	Note:	Packet sizes are usually odd-numbered because the size DOESN'T include the header byte -
	//			so the header + the packet is an even number of bytes. If you include the header byte in
	//			the packet size count (for a total even number of bytes), the resulting ID will be the same.
	const size_t	sizeID	= (packetSize >= 127) ? 0 : ((packetSize + 1) / 2);

	//	Combine the sequenceCount (bits 7-6) and the sizeID (bits 5-0)...
	const UByte		header	= ((unsigned (mPacketSequenceNum) & 0x03) << 6) + (sizeID & 0x3F);

	//	Insert header byte into Caption Channel Packet at specified index
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}
	mPacketData [index] = header;
	outNewIndex = index + dataSize;
	mPacketSequenceNum = (mPacketSequenceNum + 1) % 4;	//	Increment to the next sequence count
	return bResult;

}	//	MakeCaptionChannelPacketHeader


// MakeNullServiceBlockHeader
//		Insert a Null Service Block Header into the packet buffer at <index>
bool CNTV2CaptionEncoder708::MakeNullServiceBlockHeader (size_t index, size_t & outNewIndex)
{
	size_t	dataSize	(1);		// Null Service Block Headers are always exactly one byte

	//	Insert header byte into Caption Channel Packet at specified index
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData[index] = 0x00;
	outNewIndex = index + dataSize;
	return true;

}	//	MakeNullServiceBlockHeader


// MakeServiceBlockHeader
//		Insert a Service Block Header (Standard or Extended) into the packet buffer at <index>
bool CNTV2CaptionEncoder708::MakeServiceBlockHeader (const size_t index, int serviceNum, const size_t blockSize, size_t & outNewIndex)
{
	//	Sanity check: block sizes must be 0 - 31 bytes (NOT including the Service Block Header)
	if (blockSize > 31)
	{
		LOGMYERROR("Block Size " << blockSize << " too large (exceeds 31)");
		return false;
	}

	//	Standard Service Block headers can be used for service numbers 1-6. They are one byte long because
	//	the service number can be encoded into 3 bits. Extended Service Block headers are used when the
	//	service number is between 7 and 63. A second byte is used to hold the larger service number.

	//	Note: service number '0' is reserved and cannot be used.
	if (serviceNum == 0)
	{
		LOGMYERROR("Service Number 0 not allowed");
		return false;
	}

	if (serviceNum >= 1 && serviceNum <= 6)
	{
		const size_t	dataSize	(1);	//	Standard Service Block Headers are always exactly one byte

		//	Do a single-byte Standard Service Block header...
		UByte	header	= (UByte (serviceNum) << 5) + (UByte (blockSize) & 0x1F);	//	ms 3 bits = service number (1 - 6), ls 5 bits = block size

		//	Insert header byte into Caption Channel Packet at specified index...
		if (index > (NTV2_CC708MaxPktSize - dataSize))
		{
			LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
			return false;
		}
		mPacketData [index] = header;
		outNewIndex = index + dataSize;
	}
	else
	{
		const size_t	dataSize	(2);	//	Extended Service Block Headers are always exactly two bytes

		//	Do a two-byte Extended Service Block Header...
		UByte	hdr1	= 0xE0 + (blockSize & 0x1F);		//	set the upper 3 bits to '111' to indicate an Extended Service Block header, ls 5 bits = block size
		UByte	hdr2	= (serviceNum & 0x3F);				//	ls 6 bits = service number (7 - 63)

		//	Insert header byte into Caption Channel Packet at specified index...
		if (index > (NTV2_CC708MaxPktSize - dataSize))
		{
			LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
			return false;
		}
		mPacketData [index    ] = hdr1;
		mPacketData [index + 1] = hdr2;
		outNewIndex = index + dataSize;
	}
	return true;

}	//	MakeServiceBlockHeader


bool CNTV2CaptionEncoder708::MakeServiceBlockHeader (const size_t index, int serviceNum, const size_t blockSize)
{
	size_t	tmpNdx	(0);
	return MakeServiceBlockHeader (index, serviceNum, blockSize, tmpNdx);
}


// MakeServiceBlockCharData
//		Insert a data byte into the packet buffer at <index>
bool CNTV2CaptionEncoder708::MakeServiceBlockCharData (const size_t index, UByte data, size_t & outNewIndex)
{
	size_t	dataSize	(1);	//	Character Data is always exactly one byte

	//	Insert data byte into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData[index] = data;
	outNewIndex = index + dataSize;
	return true;

}	//	MakeServiceBlockCharData


// Set Current Window (CW0 - CW7) Command
//		Insert a "SetCurrentWindowX" command data block into the packet buffer at <index>
//		(See CEA-708B pg 40)
bool CNTV2CaptionEncoder708::MakeSetCurrentWindowCommand (const size_t index, const int windowID, size_t & outNewIndex)
{
	size_t	dataSize	(1);	//	SetCurrentWindow commands are always 1 byte

	AJACC_ASSERT (windowID >= 0 && windowID < 8 && "windowID must be between 0 and 7");
	if (windowID < 0 || windowID > 7)
	{
		LOGMYERROR("windowID " << windowID << " out of range");
		return false;
	}

	//	Insert command and parameter bytes into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData[index] = 0x80 + static_cast <UByte> (windowID);		//	The command byte is x80 + windowID...
	outNewIndex = index + dataSize;
	return true;

}	//	MakeSetCurrentWindowCommand



// Define Window (DF0 - DF7) Command
//		Insert a "DisplayWindows" command data block into the packet buffer at <index>
//		(See CEA-708B pg 41-42)
bool CNTV2CaptionEncoder708::MakeDefineWindowCommand (const size_t index, int inWindowID, const CC708WindowParms & inParms, size_t & outNewIndex)
{
	const size_t	dataSize	(7);	//	DefineWindow commands are always 7 bytes

	//	Reality check...
	if (!inParms.IsValid () || (inWindowID < NTV2_CC708WindowIDMin || inWindowID > NTV2_CC708WindowIDMax))
	{
		LOGMYERROR("range error:  inWindowID=" << inWindowID << ", inParms:  " << inParms);
		return false;
	}

	//	Insert command and parameter bytes into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	//	The command byte is x98 + the window ID
	mPacketData [index  ] = 0x98 + (inWindowID & 0x07);

	//	Param #1: '00' + v + rl + cl + priority
	mPacketData [index + 1] = (  ((inParms.visible ? 1 : 0) << 5)		// bit 5
						  + ((inParms.rowLock ? 1 : 0) << 4)		// bit 4
						  + ((inParms.colLock ? 1 : 0) << 3)		// bit 3
						  + ( inParms.priority & 0x07      ) );		// bits 2-0

	//	Param #2: relativePos + anchorV
	mPacketData [index + 2] = (  ((inParms.relativePos ? 1 : 0) << 7)	// bit 7
						  + ( inParms.anchorV & 0x7F           ) );	// bits 6-0

	//	Param #3: anchorH
	mPacketData [index + 3] = (inParms.anchorH & 0xFF);					// bits 7-0

//	Note: the CEA-708B DefineWindow command's "rowCount" and "colCount" values that are transmitted are one LESS
//	than the actual number. This method accepts the "real" counts and subtracts one.

	//	Param #4: anchorPt + rowCount
	mPacketData [index + 4] = (  ((inParms.anchorPt    & 0x0F) << 4)		// bits 7-4
						  + ((inParms.rowCount-1) & 0x0F      ) );	// bits 3-0

	//	Param #5: colCount
	mPacketData [index + 5] = ((inParms.colCount-1) & 0x3F);				// bits 5-0

	//	Param #6: windowStyleID + penStyleID
	mPacketData [index + 6] = (  ((inParms.windowStyleID & 0x07) << 3)	// bits 5-3
						  + ( inParms.penStyleID    & 0x07      ) );// bits 2-0

	outNewIndex = index + dataSize;

	return true;

}	//	MakeDefineWindowCommand


// Clear Windows (CLW) Command
//		Insert a "ClearWindows" command data block into the packet buffer at <index>
//		(See CEA-708B pg 43)
//		Note: <windowMap> is a bit-map of window enables, where bit 0 = window 0, bit 1 = window 1, etc.
//			  Setting a bit to '1' clears the corresponding window, setting a bit to '0' leaves the corresponding window as-is
bool CNTV2CaptionEncoder708::MakeClearWindowsCommand (const size_t index, UByte windowMap, size_t & outNewIndex)
{
	const size_t	dataSize	(2);	//	ClearWindows commands are always 2 bytes

	//	Insert command and parameter bytes into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData [index    ] = 0x88;		//	The command byte is x88
	mPacketData [index + 1] = windowMap;	//	Param #1: bitmap
	outNewIndex = index + dataSize;
	return true;

}	//	MakeClearWindowsCommand


// Delete Windows (DLW) Command
//		Insert a "DeleteWindows" command data block into the packet buffer at <index>
//		(See CEA-708B pg 44)
//		Note: <windowMap> is a bit-map of window enables, where bit 0 = window 0, bit 1 = window 1, etc.
//			  Setting a bit to '1' deletes the corresponding window, setting a bit to '0' leaves the corresponding window as-is
bool CNTV2CaptionEncoder708::MakeDeleteWindowsCommand (const size_t index, UByte windowEnables, size_t & outNewIndex)
{
	const size_t	dataSize	(2);	//	DeleteWindows commands are always 2 bytes

	//	Insert command and parameter bytes into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData [index    ] = 0x8C;			//	The command byte is x8C
	mPacketData [index + 1] = windowEnables;	//	Param #1: bitmap
	outNewIndex = index + dataSize;
	return true;

}	//	MakeDeleteWindowsCommand


// Display Windows (DSW) Command
//		Insert a "DisplayWindows" command data block into the packet buffer at <index>
//		(See CEA-708B pg 45)
//		Note: <windowMap> is a bit-map of window enables, where bit 0 = window 0, bit 1 = window 1, etc.
//			  Setting a bit to '1' displays the corresponding window, setting a bit to '0' leaves the corresponding window as-is
bool CNTV2CaptionEncoder708::MakeDisplayWindowsCommand (const size_t index, UByte windowMap, size_t & outNewIndex)
{
	const size_t	dataSize	(2);	//	DisplayWindows commands are always 2 bytes

	//	Insert command and parameter bytes into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData [index    ] = 0x89;		//	The command byte is x89
	mPacketData [index + 1] = windowMap;	//	Param #1: bitmap
	outNewIndex = index + dataSize;
	return true;

}	//	MakeDisplayWindowsCommand


// Hide Windows (HDW) Command
//		Insert a "HideWindows" command data block into the packet buffer at <index>
//		(See CEA-708B pg 46)
//		Note: <windowMap> is a bit-map of window enables, where bit 0 = window 0, bit 1 = window 1, etc.
//			  Setting a bit to '1' hides the corresponding window, setting a bit to '0' leaves the corresponding window as-is
bool CNTV2CaptionEncoder708::MakeHideWindowsCommand (const size_t index, UByte windowMap, size_t & outNewIndex)
{
	const size_t	dataSize	(2);	//	HideWindows commands are always 2 bytes

	//	Insert command and parameter bytes into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData [index    ] = 0x8A;		//	The command byte is x8A
	mPacketData [index + 1] = windowMap;	//	Param #1: bitmap
	outNewIndex = index + dataSize;
	return true;

}	//	MakeHideWindowsCommand


// Toggle Windows (TGW) Command
//		Insert a "ToggleWindows" command data block into the packet buffer at <index>
//		(See CEA-708B pg 47)
//		Note: <windowMap> is a bit-map of window enables, where bit 0 = window 0, bit 1 = window 1, etc.
//			  Setting a bit to '1' toggles the corresponding window, setting a bit to '0' leaves the corresponding window as-is
bool CNTV2CaptionEncoder708::MakeToggleWindowsCommand (const size_t index, UByte windowMap, size_t & outNewIndex)
{
	const size_t	dataSize	(2);	//	DisplayWindows commands are always 2 bytes

	//	Insert command and parameter bytes into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData [index    ] = 0x8B;			//	The command byte is x8B
	mPacketData [index + 1] = windowMap;		//	Param #1: bitmap
	outNewIndex = index + dataSize;
	return true;

}	//	MakeToggleWindowsCommand


// Set Window Attributes (SWA) Command
//		Insert a "SetWindowAttributes" command data block into the packet buffer at <index>
//		(See CEA-708B pg 48-49)
bool CNTV2CaptionEncoder708::MakeSetWindowAttributesCommand (const size_t index, const CC708WindowAttr & inAttr, size_t & outNewIndex)
{
	const size_t	dataSize	(5);	//	SetWindowAttributes commands are always 5 bytes

	//	Reality check...
	if (!inAttr.IsValid ())
	{
		LOGMYERROR("one or more parameters out of range");
		return false;
	}

	//	Insert command and parameter bytes into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData [index    ] = 0x97;											// The command byte is x97
																			// Param #1: foreground
	mPacketData [index + 1] = (((inAttr.fillColor.opacity & 0x03) << 6)		// bits 7-6
						  + ((inAttr.fillColor.red      & 0x03) << 4)		// bits 5-4
						  + ((inAttr.fillColor.green    & 0x03) << 2)		// bits 3-2
						  + ((inAttr.fillColor.blue     & 0x03)     ));		// bits 1-0

																			// Param #2: ls 2 bits of borderType + borderColor
	mPacketData [index + 2] = (((inAttr.borderType        & 0x03) << 6)		// bits 7-6 (note: ls 2 bits of borderType only)
						  + ((inAttr.borderColor.red    & 0x03) << 4)		// bits 5-4
						  + ((inAttr.borderColor.green  & 0x03) << 2)		// bits 3-2
						  + ((inAttr.borderColor.blue   & 0x03)     ));		// bits 1-0

																			// Param #3: ms bit of borderType + wordWrap + printDir + scrollDir + justify
	mPacketData [index + 3] = (((inAttr.borderType        & 0x04) << 5)		// bit 7   (note: ms bit of borderType only)
						  + ((inAttr.wordWrap ? 1 : 0)          << 6)		// bit 6
						  + ((inAttr.printDir           & 0x03) << 4)		// bits 5-4
						  + ((inAttr.scrollDir          & 0x03) << 2)		// bits 3-2
						  + ((inAttr.justify            & 0x03)     ));		// bits 1-0

																			// Param #4: effectSpeed + effectDir + displayEffect
	mPacketData [index + 4] = (((inAttr.effectSpeed       & 0x0F) << 4)		// bits 7-4
						  + ((inAttr.effectDir          & 0x03) << 2)		// bits 3-2
						  + ((inAttr.displayEffect      & 0x03)     ));		// bits 1-0

	outNewIndex = index + dataSize;

	return true;

}	//	MakeSetWindowAttributesCommand


// Set Pen Attributes (SPA) Command
//		Insert a "SetPenAttributes" command data block into the packet buffer at <index>
//		(See CEA-708B pg 50-51)
bool CNTV2CaptionEncoder708::MakeSetPenAttributesCommand (const size_t index, const CC708PenAttr & inAttr, size_t & outNewIndex)
{
	const size_t	dataSize	(3);	//	SetPenAttributes commands are always 3 bytes

	//	Reality check...
	if (!inAttr.IsValid ())
	{
		LOGMYERROR ("pen attributes " << inAttr << " out of range");
		return false;
	}

	//	Insert command and parameter bytes into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData [index    ] = 0x90;									//	The command byte is x90
																	//	Param #1: textTag + offset + penSize
	mPacketData [index + 1] = (((inAttr.textTag & 0x0F) << 4)		// bits 7-4
						  + ( (inAttr.offset  & 0x03) << 2)			// bits 3-2
						  + ( (inAttr.penSize & 0x03)     ));		// bits 1-0
																	//	Param #2: italics + underline + edgeType + fontStyle
	mPacketData [index + 2] = (((inAttr.italics   ? 1 : 0) << 7)	// bit 7
						  + ( (inAttr.underline ? 1 : 0) << 6)		// bit 6
						  + ( (inAttr.edgeType  & 0x07)  << 3)		// bits 5-3
						  + ( (inAttr.fontStyle & 0x07)      ));	// bits 2-0
	outNewIndex = index + dataSize;

	return true;

}	//	MakeSetPenAttributesCommand


// Set Pen Color (SPC) Command
//		Insert a "SetPenColor" command data block into the packet buffer at <index>
//		(See CEA-708B pg 52)
bool CNTV2CaptionEncoder708::MakeSetPenColorCommand (const size_t index, const CC708PenColor & inColor, size_t & outNewIndex)
{
	const size_t	dataSize	(4);	//	SetPenColor commands are always 4 bytes

	//	Reality check...
	if (!inColor.IsValid ())
	{
		LOGMYERROR("pen color " << inColor << " out of range");
		return false;
	}

	//	Insert command and parameter bytes into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData [index    ] = 0x91;			//	The command byte is x91
	//	Param #1: foreground opacity/color
	mPacketData [index + 1] = (((inColor.fg.opacity & 0x03) << 6)	// bits 7-6
						  + ((inColor.fg.red      & 0x03) << 4)		// bits 5-4
						  + ((inColor.fg.green    & 0x03) << 2)		// bits 3-2
						  + ((inColor.fg.blue     & 0x03)     ));	// bits 1-0
	//	Param #2: background opacity/color
	mPacketData [index + 2] = (((inColor.bg.opacity & 0x03) << 6)	// bits 7-6
						  + ((inColor.bg.red      & 0x03) << 4)		// bits 5-4
						  + ((inColor.bg.green    & 0x03) << 2)		// bits 3-2
						  + ((inColor.bg.blue     & 0x03)     ));	// bits 1-0
	//	Param #3: edge color
	mPacketData [index + 3] = (  									// bits 7-6 = 00
						  + ((inColor.edge.red    & 0x03) << 4)		// bits 5-4
						  + ((inColor.edge.green  & 0x03) << 2)		// bits 3-2
						  + ((inColor.edge.blue   & 0x03)     ));	// bits 1-0
	outNewIndex = index + dataSize;
	return true;

}	//	MakeSetPenColorCommand



// Set Pen Location (SPL) Command
//		Insert a "SetPenLocation" command data block into the packet buffer at <index>
//		(See CEA-708B pg 53)
bool CNTV2CaptionEncoder708::MakeSetPenLocationCommand (const size_t index, const CC708PenLocation & inLoc, size_t & outNewIndex)
{
	const size_t	dataSize	(3);	//	SetPenLocation commands are always 3 bytes

	//	Reality check...
	if (!inLoc.IsValid ())
	{
		LOGMYERROR("pen location " << inLoc << " out of range");
		return false;
	}

	//	Insert command and parameter bytes into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData [index    ] = 0x92;					//	The command byte is x92
	mPacketData [index + 1] = (inLoc.row    & 0x0F);	//	Param #1: row		bits 3-0
	mPacketData [index + 2] = (inLoc.column & 0x3F);	//	Param #2: col		bits 5-0
	outNewIndex = index + dataSize;
	return true;

}	//	MakeSetPenLocationCommand



// Delay (DLY) Command
//		Insert a "Delay" command data block into the packet buffer at <index>
//		(See CEA-708B pg 54)
bool CNTV2CaptionEncoder708::MakeDelayCommand (const size_t index, const UByte delay, size_t & outNewIndex)
{
	const size_t	dataSize(2);	//	Delay commands are always 2 bytes

	//	Insert command and parameter bytes into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData [index] = 0x8D;			//	The command byte is 0x8D
	mPacketData [index + 1] = delay;	//	Param #1: bitmap
	outNewIndex = index + dataSize;
	return true;

}	//	MakeDelayCommand


// DelayCancel (DLC) Command
//		Insert a "DelayCancel" command data block into the packet buffer at <index>
//		(See CEA-708B pg 55)
bool CNTV2CaptionEncoder708::MakeDelayCancelCommand (const size_t index, size_t & outNewIndex)
{
	const size_t	dataSize	(1);	//	DelayCancel commands are always 1 byte

	//	Insert command and parameter bytes into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData [index] = 0x8E;		//	The command byte is x8E
	outNewIndex = index + dataSize;
	return true;

}	//	MakeDelayCancelCommand


// Reset (RST) Command
//		Insert a "Reset" command data block into the packet buffer at <index>
//		(See CEA-708B pg 56)
bool CNTV2CaptionEncoder708::MakeResetCommand (const size_t index, size_t & outNewIndex)
{
	const size_t	dataSize	(1);	//	Reset commands are always 1 byte

	//	Insert command and parameter bytes into Caption Channel Packet at specified index...
	if (index > (NTV2_CC708MaxPktSize - dataSize))
	{
		LOGMYERROR("index " << index << " out of range (exceeds " << (NTV2_CC708MaxPktSize-dataSize) << ")");
		return false;
	}

	mPacketData [index] = 0x8F;		//	The command byte is x8F
	outNewIndex = index + dataSize;
	return true;

}	//	MakeResetCommand





//********************************************************************************************
//
//	These methods are used for building higher-level SMPTE 334 Ancillary Packets in the 
//	mAnc334Data buffer, using the Caption Channel Packet data generated in mPacketData
//
//********************************************************************************************


// Clear608CaptionData
//		Initialize 608 caption local memory
//
bool CNTV2CaptionEncoder708::Clear608CaptionData (void)
{
	bool	bResult	(true);

	m608CaptionData.bGotField1Data = false;
	m608CaptionData.f1_char1 = 0x80;
	m608CaptionData.f1_char2 = 0x80;
	
	m608CaptionData.bGotField2Data = false;
	m608CaptionData.f2_char1 = 0x80;
	m608CaptionData.f2_char2 = 0x80;
	
	m608CaptionData.bGotField3Data = false;
	m608CaptionData.f3_char1 = 0x80;
	m608CaptionData.f3_char2 = 0x80;
	
	return bResult;

}	//	Clear608CaptionData



// Set608CaptionData
//		Copy the designated 608 caption data into local memory
//
bool CNTV2CaptionEncoder708::Set608CaptionData (const CaptionData & inCC608Data)
{
	//	Make a local copy of the 608 data...
	m608CaptionData = inCC608Data;	//	struct copy
	if (inCC608Data.HasData())
		LOGMYNOTE(m608CaptionData);
	return true;

}	//	Set608CaptionData


// Set608FieldCaptionData
//		Copy the designated 608 caption data into local memory
//
bool CNTV2CaptionEncoder708::Set608CaptionData (const NTV2Line21Field inField, const UByte inChar1, const UByte inChar2, const bool inGotData)
{
	UByte	char1 (inChar1), char2 (inChar2);

	if (inField == NTV2_CC608_Field1)
	{
		m608CaptionData.f1_char1 = char1;
		m608CaptionData.f1_char2 = char2;
		m608CaptionData.bGotField1Data = inGotData;
	}
	else if (inField == NTV2_CC608_Field2)
	{
		m608CaptionData.f2_char1 = inChar1;
		m608CaptionData.f2_char2 = char2;
		m608CaptionData.bGotField2Data = inGotData;
	}
	if (m608CaptionData.HasData())
		LOGMYNOTE(m608CaptionData << " set from " << ::NTV2Line21FieldToStr(inField) << "|x" << UHEX2(inChar1) << "|x" << UHEX2(inChar2));
	return true;

}	//	Set608CaptionData


// MakeSMPTE334AncPacket
//		Build a SMPTE 334 Ancillary Packet
//
bool CNTV2CaptionEncoder708::MakeSMPTE334AncPacket (const NTV2FrameRate inFrameRate, const NTV2Line21Field inVideoField)
{
	//	Init the ANC buffer...
	::memset (mAnc334Data, 0, sizeof(mAnc334Data));
	mAnc334Size = 0;

	size_t			ancIndex	(0);
	const size_t	ancStart	(ancIndex);						//	Remember where we started the SMPTE 334 Anc packet

	if (!InsertSMPTE334AncHeader(ancIndex, 0))					//	We'll come back later and fix the DataCount
		{LOGMYERROR("InsertSMPTE334AncHeader failed"); return false;}

	const size_t	cdpStartIndex	(ancIndex);					//	Remember where we started the CaptionDataPacket
	if (!InsertCDPHeader(ancIndex, inFrameRate, mCDPSequenceNum))
		{LOGMYERROR("InsertCDPHeader failed for " << ::NTV2FrameRateToString(inFrameRate)
					<< ", ancNdx=" << DEC(ancIndex) << ", CDPSeqNum=" << DEC(mCDPSequenceNum)); return false;}

	if (!InsertCDPData(ancIndex, inFrameRate, inVideoField))
		{LOGMYERROR("InsertCDPData failed for " << ::NTV2FrameRateToString(inFrameRate)
					<< ", ancNdx=" << DEC(ancIndex) << ", field=" << DEC(inVideoField)); return false;}

	if (mServiceInfo.NumActiveCDPServiceInfo())
	{
		const size_t	ancSvcInfoStart(ancIndex);				//	Remember where we started this section

		if (!InsertCDPServiceInfo(ancIndex))
			{LOGMYERROR("InsertCDPServiceInfo failed at ancNdx=" << DEC(ancIndex)); return false;}

		//	The svc_info status flags (Start/Change/Complete) must be copied back into the cdp_header...
		const UByte	svcInfoFlags(mAnc334Data[ancSvcInfoStart+1] & 0x70);							//	Bits 4-6 of the svc_info status word
		mAnc334Data[cdpStartIndex+4] |= ((svcInfoFlags >> 2) + NTV2_CC708CDPHeader_SvcInfoPresent);	//	Shift to bits 2-4 and add the "svc_info present" flag

		//	The CDP length must also be updated -- increment it by the size of the ccsvcinfo_section...
		const size_t	svcCount		(mAnc334Data[ancSvcInfoStart+1] & 0x0F);
		const UWord		svcInfoLength	(2 + (7 * static_cast<UWord>(svcCount)));
		mAnc334Data[cdpStartIndex+2] += svcInfoLength;
	}	//	if one or more services are active

	//	Caption Data Packet checksum and sequence number...
	if (!InsertCDPFooter(ancIndex, cdpStartIndex, mCDPSequenceNum))
		{LOGMYERROR("InsertCDPFooter failed at ancIndex=" << DEC(ancIndex) << ", CDPStartNdx=" << DEC(cdpStartIndex)
					<< ", CDPSeqNum=" << DEC(mCDPSequenceNum)); return false;}

	//	Calculate total number of words in SMPTE 334 UDW section and update the SMPTE 334 Header...
	const UByte	UDWSize	(static_cast<UByte>(ancIndex - ancStart - 6));
	CNTV2SMPTEAncData::SetAncHeaderDataCount(reinterpret_cast<NTV2_SMPTEAncHeaderPtr>(&mAnc334Data[ancStart]), UDWSize);

	//	Calculate and insert the SMPTE 334 Anc checksum...
	if (!InsertSMPTE334AncFooter(ancIndex, ancStart))
		{LOGMYERROR("InsertSMPTE334AncFooter failed at ancIndex=" << DEC(ancIndex)
					<< ", ancStart=" << DEC(ancStart)); return false;}

	mAnc334Size = ancIndex;	//	Finalize the packet size
	mCDPSequenceNum += 1;	//	Bump the sequence count so the next one will be different
	return true;			//	Success!

}	//	MakeSMPTE334AncPacket


// MakeSMPTE334AncPacket
//		Build a SMPTE 334 Ancillary Packet and returns a ptr and size
//
bool CNTV2CaptionEncoder708::MakeSMPTE334AncPacket (const NTV2FrameRate frameRate, const NTV2Line21Field field, UWordPtr & outAncPacketData, size_t & outSize)
{
	const bool	result	(MakeSMPTE334AncPacket (frameRate, field));
	outAncPacketData = mAnc334Data;
	outSize = mAnc334Size;
	return result;

}	//	MakeSMPTE334AncPacket



// MakeSMPTE334AncPacketFromCDP
//		Build a SMPTE 334 Ancillary Packet using the supplied CDP and returns a ptr and size
//		Note: assumes that the supplied 8-bit CDP is correct and intact and only needs parity added.
//
bool CNTV2CaptionEncoder708::MakeSMPTE334AncPacketFromCDP (const UBytePtr pCDP, const size_t cdpLength, UWordPtr & outAncPacketData, size_t & outSize)
{
	bool	bResult	(false);

	//	Clear my Anc buffer...
	::memset (mAnc334Data, 0, sizeof (mAnc334Data));
	mAnc334Size = 0;

	size_t			ancIndex	(0);
	const size_t	ancStart	(ancIndex);				//	Remember where we started the SMPTE 334 Anc packet

	//	SMPTE-334 Anc header
	bResult = InsertSMPTE334AncHeader (ancIndex, 0);	//	We'll come back later and fix the DataCount

	//	Copy the supplied CDP, adding parity...
	for (size_t i = 0; i < cdpLength; i++)
		mAnc334Data [ancIndex++] = CNTV2SMPTEAncData::AddEvenParity (pCDP [i]);

	//	Calculate total number of words in SMPTE 334 UDW section and update the SMPTE 334 Header...
	const UByte	UDWSize	(static_cast <UByte> (ancIndex - ancStart - 6));
	CNTV2SMPTEAncData::SetAncHeaderDataCount (reinterpret_cast <NTV2_SMPTEAncHeaderPtr> (&mAnc334Data [ancStart]), UDWSize);

	//	Calculate and insert the SMPTE 334 Anc checksum...
	bResult = InsertSMPTE334AncFooter (ancIndex, ancStart);
	mAnc334Size = ancIndex;

	//	Return results...
	outAncPacketData = mAnc334Data;
	outSize = mAnc334Size;

	return bResult;

}	//	MakeSMPTE334AncPacketFromCDP


bool CNTV2CaptionEncoder708::MakeSMPTE334AncPacketFromCDP (const UBytePtr pCDP, const size_t cdpLength)
{
	UWordPtr	pAncPacketData	(NULL);
	size_t		ancPacketSize	(0);

	return MakeSMPTE334AncPacketFromCDP (pCDP, cdpLength, pAncPacketData, ancPacketSize);

}	//	MakeSMPTE334AncPacketFromCDP


UWordSequence CNTV2CaptionEncoder708::GetSMPTE334DataVector (void) const
{
	UWordSequence	result;
	for (size_t ndx(0);  ndx < GetSMPTE334Size()  &&  ndx < NTV2_CC708MaxAncSize;  ndx++)
		result.push_back(mAnc334Data[ndx]);
	return result;
}


// InsertSMPTE334AncHeader
//		Build a SMPTE 334 Ancillary Header in mAnc334Data at <ancIndex>
//
bool CNTV2CaptionEncoder708::InsertSMPTE334AncHeader (size_t & inOutAncIndex, const UByte inDataCount)
{
	const size_t	dataSize	(6);

	if (inOutAncIndex > (NTV2_CC708MaxPktSize - dataSize))
		return false;

	const bool	bResult	(CNTV2SMPTEAncData::MakeAncHeader (reinterpret_cast <NTV2_SMPTEAncHeaderPtr> (&mAnc334Data [inOutAncIndex]),
															NTV2_SMPTEAncRP334DID,   NTV2_SMPTEAncRP334SDID,   inDataCount));
	inOutAncIndex += dataSize;
	return bResult;

}	//	InsertSMPTE334AncHeader



// AddSMPTE334AncFooter
//		Build a SMPTE 334 Ancillary Footer in mAnc334Data.
//		<inAncStartIndex> must point to the beginning of the complete anc packet header.
//
// Note: this method assumes that the SMPTE 334 ancillary packet has already been built and is correctly sized
//       (not including the checksum).
//
bool CNTV2CaptionEncoder708::InsertSMPTE334AncFooter (size_t & inOutAncIndex, const size_t inAncStartIndex)
{
	bool 			bResult		(false);
	const size_t	dataSize	(1);

	//	Sanity check...
	if (inOutAncIndex <= NTV2_CC708MaxPktSize - dataSize)
	{
		bResult = CNTV2SMPTEAncData::CalculateAncChecksum ((NTV2_SMPTEAncHeaderPtr) &mAnc334Data [inAncStartIndex], 0);
		inOutAncIndex += dataSize;
	}

	return bResult;

}	//	InsertSMPTE334AncFooter



// InsertCDPHeader
//		Build a CEA-708B Caption Data Pack cdp_header section (see CEA-708B - page 71)
//
bool CNTV2CaptionEncoder708::InsertCDPHeader (size_t & inOutAncIndex, const NTV2FrameRate frameRate, const int cdpSeqNum)
{
	bool 			bResult		(false);
	const size_t	dataSize	(7);

	//	Sanity check...
	if (inOutAncIndex <= NTV2_CC708MaxPktSize - dataSize)
	{
		//	CDP Header ID
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (NTV2_CC708_CDPHeaderId1);	//	cdp_identifier (ms)
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (NTV2_CC708_CDPHeaderId2);	//	cdp_identifier (ls)

		//	Convert NTV2 frame rate to CEA-708...
		size_t	cdpFrameRate	(0);
		size_t	numCCTriplets	(0);
		size_t	num608Triplets	(0);
		bResult = ConvertFrameRate (frameRate, cdpFrameRate, numCCTriplets, num608Triplets);

		//	CDP length
		const UByte	length	(7 + 2 + (3 * static_cast <UByte> (numCCTriplets)) + 4);	//	header + cc_data + footer (caller must add the ccsvcinfo_section length later)
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (length);		//	cdp_length

		//	CDP Frame Rate
		cdpFrameRate = ((cdpFrameRate & 0x0F) << 4) + 0x0F;								//	bits 7:4 = frame rate, bits 3:0 = '1111'
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (static_cast <UByte> (cdpFrameRate));	//	cdp_frame_rate

		//	CDP Misc Flags
		//	Note that we're assuming we ALWAYS have a Data Section and NEVER have a TimeCode Section...
		//	Changed: the caller needs to wait for the ccsvcinfo_section to be added and then copy the status bits from there.
		const UByte	cdpFlags =   NTV2_CC708CDPHeader_CCDataPresent
							   + NTV2_CC708CDPHeader_CaptionServiceActive
							   + NTV2_CC708CDPHeader_Reserved;
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (cdpFlags);		// header misc bits

		//	CDP Sequence Number
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity ((cdpSeqNum >> 8) & 0xFF);	// cdp_hdr_sequence_ctr (ms)
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity ((cdpSeqNum     ) & 0xFF);	// cdp_hdr_sequence_ctr (ls)
	}

	return bResult;

}	//	InsertCDPHeader


// InsertCDPFooter
//		Build a CEA-708B Caption Data Pack cdp_footer section (see CEA-708B - page 77)
//
bool CNTV2CaptionEncoder708::InsertCDPFooter (size_t & inOutAncIndex, const size_t cdpStartIndex, const int cdpSeqNum)
{
	const size_t	dataSize	(4);

	//	Sanity check...
	if (inOutAncIndex <= NTV2_CC708MaxPktSize - dataSize)
	{
		//	CDP Footer ID
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (NTV2_CC708_CDPFooterId);	// cdp_footer_id

		//	CDP Sequence Number (note: this should match the number we put in the CDP Header)
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity ((cdpSeqNum >> 8) & 0xFF);	// cdp_ftr_sequence_ctr (ms)
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity ((cdpSeqNum     ) & 0xFF);	// cdp_ftr_sequence_ctr (ls)

		//	CDP checksum: according to CEA-708B (pg 77), the packet_checksum is "the 8-bit value necessary to
		//	make the arithmetic sum of the entire packet (first byte of cdp_identifier to packet_checksum,
		//	inclusive) modulo 256 equal zero."
		//	In other words, we need to sum all of the packet words from <cdpStartIndex> through <inOutAncIndex+2>
		//	and subtract from 256.
		int		checksum	(0);
		size_t	csIndex		(cdpStartIndex);
		while (csIndex <= inOutAncIndex + 2)
			checksum += mAnc334Data [csIndex++];	//	NOTE:	We can ignore parity bits because they are > 255 and will get masked off
		
		checksum = (((checksum & 0xFF) == 0) ? 0 : 256 - (checksum & 0xFF));
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (static_cast <UByte> (checksum));	//	Packet_checksum
	}

	return true;

}	//	InsertCDPFooter


// InsertCDPData
//		Build a CEA-708B Caption Data Pack ccdata_section (see CEA-708B - page 74)
//
bool CNTV2CaptionEncoder708::InsertCDPData (size_t & inOutAncIndex, const NTV2FrameRate inFrameRate, const NTV2Line21Field inVideoField)
{
	bool	bResult	(false);

	//	Convert NTV2 frame rate to CEA-708
	size_t	ccCount			(0);	//	Total number of triplets
	size_t	cc608Count		(0);	//	Of those, number of 608 triplets
	size_t	tmpConverted	(0);
	bResult = ConvertFrameRate (inFrameRate, tmpConverted, ccCount, cc608Count);

	//	The total data size is 2 + (3 * ccCount)...
	const size_t	dataSize	(2 + (3 * ccCount));

	//	Sanity check...
	if (inOutAncIndex <= NTV2_CC708MaxPktSize - dataSize)
	{
		//	CDP Data ID
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (NTV2_CC708_CDPDataId);	// ccdata_id

		//	ccCount (bits 4:0) + '111' (bits 7:5)
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (0xE0 + (ccCount & 0x1F));	// cc_count

		if (cc608Count > 1)
		{
			//	Insert 608 triplets first...
			if (cc608Count > 0)		//	608 field 1
				bResult = InsertCDPDataTriplet (inOutAncIndex, m608CaptionData.bGotField1Data, NTV2_CC708CCTypeNTSCField1, m608CaptionData.f1_char1, m608CaptionData.f1_char2);

			if (cc608Count > 1)		//	608 field 2
				bResult = InsertCDPDataTriplet (inOutAncIndex, m608CaptionData.bGotField2Data, NTV2_CC708CCTypeNTSCField2, m608CaptionData.f2_char1, m608CaptionData.f2_char2);

			if (cc608Count > 2)		//	Note:	This only happens when 30 fps NTSC with 3:2 pulldown has been upconverted to a 24 fps format
				bResult = InsertCDPDataTriplet (inOutAncIndex, m608CaptionData.bGotField3Data, NTV2_CC708CCTypeNTSCField1, m608CaptionData.f3_char1, m608CaptionData.f3_char2);
		}
		else if (cc608Count == 1)
		{
			//	Only using one field - which one?
			if (inVideoField == NTV2_CC608_Field2)
				bResult = InsertCDPDataTriplet (inOutAncIndex, m608CaptionData.bGotField2Data, NTV2_CC708CCTypeNTSCField2, m608CaptionData.f2_char1, m608CaptionData.f2_char2);
			else
				bResult = InsertCDPDataTriplet (inOutAncIndex, m608CaptionData.bGotField1Data, NTV2_CC708CCTypeNTSCField1, m608CaptionData.f1_char1, m608CaptionData.f1_char2);
		}
		else if (m608CaptionData.bGotField1Data || m608CaptionData.bGotField2Data)
			LOGMYWARN("CEA608 caption data present, but CEA708 stipulates zero CEA608 data triplets of " << DEC(ccCount) << " (max) for " << ::NTV2FrameRateToString(inFrameRate));

		//	Decrease the total number of triplets by the number of 608 triplets...
		ccCount -= cc608Count;

		//	The remainder is 708 data -- get it from mPacketData...
		size_t	pktIndex	(0);
		int ccType = NTV2_CC708CCTypeDTVCCStart;	//	This type should only be used for the FIRST triplet

		for (size_t ccNdx = 0;  ccNdx < ccCount;  ccNdx++)
		{
			UByte data1  = 0;
			UByte data2  = 0;
			bool ccValid = false;

			//	Got first data byte?
			if (pktIndex < mPacketDataSize)
			{
				ccValid = true;
				data1 = mPacketData [pktIndex++];
			}

			//	Got second data byte?
			if (pktIndex < mPacketDataSize)
				data2 = mPacketData [pktIndex++];

			//	If there is NO data in the first triplet (i.e. the first triplet is 0/0), don't use the "Packet Start" flag
			if (ccType == NTV2_CC708CCTypeDTVCCStart && ccValid == false)
				ccType = NTV2_CC708CCTypeDTVCCData;

			bResult = InsertCDPDataTriplet (inOutAncIndex, ccValid, ccType, data1, data2);

			ccType = NTV2_CC708CCTypeDTVCCData;		//	This type should be used for all triplets after the first one
		}
	}

	return bResult;

}	//	InsertCDPData


// InsertCDPDataTriplet
//		Insert a 3-word data "triplet" into the cc_data section of a Caption Data Packet at <ancIndex>. (see CEA-708B - page 75)
//
bool CNTV2CaptionEncoder708::InsertCDPDataTriplet (size_t & inOutAncIndex, const bool ccValid, const int ccType, const UByte data1, const UByte data2)
{
	const size_t	dataSize	(3);	//	Each triplet is (duh) 3 words

	//	Sanity check...
	if (inOutAncIndex <= NTV2_CC708MaxPktSize - dataSize)
	{
		// Word #0: '11111' (bits 7:3) + ccValid (bit 2) + ccType (bits 1:0)
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity ( (0xF8 + (ccValid ? 0x04 : 0) + (ccType & 0x03)) );	// cc_valid + cc_type

		// Word #1: data1
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (data1);												// cc_data_1

		// Word #2: data2
		mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (data2);												// cc_data_2
	}

	return true;

}	//	InsertCDPDataTriplet


// ConvertFrameRate
//		Convert an NTV2FrameRate enum into a CEA-708B "cdp_frame_rate" (see CEA-708B - page 74), and report the
//		number of 608 data triplets and total data triplets contained in a Caption Data Packet for this frame rate.
//
bool CNTV2CaptionEncoder708::ConvertFrameRate (const NTV2FrameRate inNTV2FrameRate, size_t & outConverted, size_t & outNumCCTriplets, size_t & outNum608Triplets)
{
	bool	bResult	(true);

	outConverted = 0;
	outNumCCTriplets = 0;	// total number of data triplets per frame (both 608 and 708 data)
	outNum608Triplets = 0;	// number of 608 data triplets per frame

	switch (inNTV2FrameRate)
	{
		case NTV2_FRAMERATE_2398:	outConverted = NTV2_CC708CDPFrameRate23p98;	outNumCCTriplets = 25;	outNum608Triplets = 3;		break;
		case NTV2_FRAMERATE_2400:	outConverted = NTV2_CC708CDPFrameRate24;	outNumCCTriplets = 25;	outNum608Triplets = 3;		break;
		case NTV2_FRAMERATE_2500:	outConverted = NTV2_CC708CDPFrameRate25;	outNumCCTriplets = 24;	outNum608Triplets = 0;		break;
		case NTV2_FRAMERATE_2997:	outConverted = NTV2_CC708CDPFrameRate29p97;	outNumCCTriplets = 20;	outNum608Triplets = 2;		break;
		case NTV2_FRAMERATE_3000:	outConverted = NTV2_CC708CDPFrameRate30;	outNumCCTriplets = 20;	outNum608Triplets = 2;		break;
		case NTV2_FRAMERATE_5000:	outConverted = NTV2_CC708CDPFrameRate50;	outNumCCTriplets = 12;	outNum608Triplets = 0;		break;
		case NTV2_FRAMERATE_5994:	outConverted = NTV2_CC708CDPFrameRate59p94;	outNumCCTriplets = 10;	outNum608Triplets = 1;		break;
		case NTV2_FRAMERATE_6000:	outConverted = NTV2_CC708CDPFrameRate60;	outNumCCTriplets = 10;	outNum608Triplets = 1;		break;
		default:					bResult = false;	break;		//	That's all that CEA-708B defines - so the rest are an error...?
	}

	return bResult;

}	//	ConvertFrameRate


// InsertCDPServiceInfo
//		Build a CEA-708B Caption Data Pack ccsvcinfo_section (see CEA-708B - page 75)
//
bool CNTV2CaptionEncoder708::InsertCDPServiceInfo (size_t & inOutAncIndex)
{
	bool	bStartFlag	(false);	//	Will be set if we're starting a new round of svc_info updates
	bool	bEndFlag	(false);	//	Will be set if we're finishing a round of svc_info updates
	bool	bChangeFlag	(false);	//	Will be set if the svc_info database has it's Change flag set

	//	Count the TOTAL number of active services:
	//	-	If the answer is zero, we don't have to add a service_info section at all
	//	-	If the answer is less than or equal to the number of services we're willing/able
	//		to pack into one CDP, we can do all of them at once
	//	-	If the answer is greater than the max number we're willing to pack into one CDP,
	//		we need to output groups of them on successive frames
	int totalSvcInfoCount = mServiceInfo.NumActiveCDPServiceInfo ();

	//	If we don't have ANY services to output, just fuhgetaboutit...
	if (totalSvcInfoCount > 0)
	{
		//	Safety check: make sure our local "services per CDP" value is legal...
		if (mNumServiceInfoPerCDP > NTV2_CC708MaxCDPServices)
			mNumServiceInfoPerCDP = NTV2_CC708MaxCDPServices;

		//	Our mServiceInfo database keeps track of where we left off after outputting the last CDP.
		//	Find out how many active services are left after (but including) the current service index...
		int currSvcIndex = mServiceInfo.GetStartIndex ();
		int svcInfoCount = mServiceInfo.NumActiveCDPServiceInfo (currSvcIndex);

		//	This shouldn't happen: if the TOTAL count is greater than zero, but there are none past
		//	(or including) the current index, then we need to reset and get them back in sync...
		if (svcInfoCount <= 0)
		{
			mServiceInfo.ResetStartIndex ();
			currSvcIndex = mServiceInfo.GetStartIndex ();
			svcInfoCount = mServiceInfo.NumActiveCDPServiceInfo (currSvcIndex);
		}

		//	Are there more active services remaining than we are willing to output in one CDP?
		if (svcInfoCount > mNumServiceInfoPerCDP)
			svcInfoCount = mNumServiceInfoPerCDP;	//	Yes -- we're only going to send a portion of the remaining active services
		else
			bEndFlag = true;						//	No -- all the (remaining) services will fit in this CDP

		//	Now that we know how many svc_info thingies we want to output, make sure we have the space remaining
		//	in the CDP to insert them...
		size_t dataSize = 2 + size_t (svcInfoCount * 7);	//	Total size (in words) of svcInfo_section

		//	Sanity check...
		if (svcInfoCount > 0  &&  inOutAncIndex <= NTV2_CC708MaxPktSize - dataSize)
		{
			//	OK -- we have something to output, and a place to put it
			//	Special case: if currSvcIndex is zero, it means we are starting a fresh round of service_info updates...
			if (currSvcIndex == 0)
			{
				bStartFlag = true;
				bChangeFlag = mServiceInfo.GetServiceInfoChangeFlag ();	//	The status of the Change flag is supposed to be synchronous with the beginning of a new round
			}

			//	CDP Service ID
			mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (NTV2_CC708_CDPServiceInfoId);		// ccsvcinfo_id

			//	Marker bit ('1') + svc_info_start/change/complete (bits 6:4) + svc_count (bits 3:0)
			//	NOTE:	The Start/Change/Complete bits also need to be copied into the cdp_header...
			mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (  0x80
																				+ (bStartFlag  ? NTV2_CC708CDPSvcInfo_SvcInfoStart    : 0)
																				+ (bChangeFlag ? NTV2_CC708CDPSvcInfo_SvcInfoChange   : 0)
																				+ (bEndFlag    ? NTV2_CC708CDPSvcInfo_SvcInfoComplete : 0)
																				+ (svcInfoCount & 0x0F) );

			//	Get the "startIndex" of the mServiceInfo database...
			//	subtle point:	If the startIndex is already pointing to an active service, it will stay put.
			//               	If not, it will advance until it gets to the first active service.
			currSvcIndex = mServiceInfo.AdvanceToNextStartIndex (true);

			//	For each service...
			for (int j = 0; j < svcInfoCount; j++)
			{
				//	'111' (bits 7:5) + caption_service_number (bits 4:0)
				int captionSvcNumber = mServiceInfo.GetCaptionServiceNumber (currSvcIndex);
				mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (0xE0 + (captionSvcNumber & 0x1F));

				//	3 characters of "language ID"...
				NTV2_CC708ServiceLanguage	lang;
				mServiceInfo.GetServiceInfoLanguage (currSvcIndex, lang);
				mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (UByte (lang.langID [0]));
				mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (UByte (lang.langID [1]));
				mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (UByte (lang.langID [2]));

				//	Caption_service_number...
				if (mServiceInfo.GetServiceInfoDigitalCC (currSvcIndex))
					mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (NTV2_CC708CDPSvcInfo_DigitalCC + 0x40 + (captionSvcNumber & 0x3F));
				else
					mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (0x40 + 0x3E);

				//	Easy_reader + wide_aspect_ratio flags...
				UByte flag1 = (  (mServiceInfo.GetServiceInfoEasyReader (currSvcIndex) ? NTV2_CC708CDPSvcInfo_EasyReader      : 0)
							   + (mServiceInfo.GetServiceInfoWideAspect (currSvcIndex) ? NTV2_CC708CDPSvcInfo_WideAspectRatio : 0) );
				mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (flag1 + 0x3F);

				//	Reserved
				mAnc334Data [inOutAncIndex++] = CNTV2SMPTEAncData::AddEvenParity (0xFF);

				//	Advance to next active service...
				currSvcIndex = mServiceInfo.AdvanceToNextStartIndex (false);

			}	// for (int j = 0; j < svcInfoCount; j++)

		}	// if (svcInfoCount > 0 && ancIndex <= NTV2_CC708MaxPktSize - dataSize)

	}	// if (totalSvcInfoCount > 0)

	return true;

}	//	InsertCDPServiceInfo



// CC608OddParity()
//		Add odd parity to 608 character
//
UByte CNTV2CaptionEncoder708::CC608OddParity (UByte ch)
{
	UByte	result	(ch);
	int		ones	(0);

	//	Count the number of ones in the ls 7 bits...
	for (int i = 0; i < 7; i++)
	{
		if (ch & 0x01)
			ones++;

		ch = ch >> 1;
	}

	//	If there are an even number of ones, add another in bit 7...
	if (ones % 2 == 0)
		result |= 0x80;
	else
		result &= 0x7F;

	return result;

}	//	CC608OddParity


// Note:  There's one way to output SMPTE-334 Anc data, and one method to handle it:
//	You have a video frame (with VANC area) in memory and you want the SMPTE-334 Anc packet inserted into
//	   the frame buffer, call InsertSMPTE334AncPacketInVideoFrame().
//	The SMPTE-334 Anc data is expected to already be created and formatted in mAnc334Data.


// InsertSMPTE334AncPacketInVideoFrame()
//
bool CNTV2CaptionEncoder708::InsertSMPTE334AncPacketInVideoFrame (void * pFrameBuffer,
																  const NTV2VideoFormat inVideoFormat,
																  const NTV2FrameBufferFormat inPixelFormat,
																  const ULWord inVancLineNum,
																  const ULWord inWordOffset) const
{
	return CNTV2SMPTEAncData::InsertAnc (mAnc334Data, mAnc334Size,						//	Pointer to Anc buffer, size of Anc data
										 inVancLineNum, inWordOffset,					//	Target line number and word (channel) offset in host FB
										 reinterpret_cast <ULWord *> (pFrameBuffer),	//	Pointer to host FB
										 inVideoFormat, inPixelFormat);					//	Host video format & FB pixel format

}	//	InsertSMPTE334AncPacketInVideoFrame


// SetServiceInfoActive()
//
bool CNTV2CaptionEncoder708::SetServiceInfoActive (int svcIndex, bool bActive)
{
	return mServiceInfo.SetServiceInfoActive (svcIndex, bActive);

}	//	SetServiceInfoActive


// CopyAllServiceInfo()
//	This is used to copy an entire ServiceInfo database to the encoder, for example copying
//	the ServiceInfo from a 708 decoder channel into the encoder for the purposes of translation.
//	Returns 'true' if there has been a change in the service info data
//
bool CNTV2CaptionEncoder708::CopyAllServiceInfo (const NTV2_CC708ServiceData & inSrcSvcData)
{
	return mServiceInfo.CopyAllServiceInfo (inSrcSvcData);

}	//	CopyAllServiceInfo


// SetDebugLevel()
//		Determines what kind of debug printf's get sent to console
//
NTV2CaptionLogMask CNTV2CaptionEncoder708::SetLogMask (const NTV2CaptionLogMask inLogMask)
{
	mServiceInfo.SetLogMask (inLogMask);
	return CNTV2CaptionLogConfig::SetLogMask (inLogMask);
}

void NTV2_CC708CDPDataSection::Zero (void)
{
	ccdata_id = 0; cc_count = 0;
	for (int ndx(0);  ndx < 32;  ndx++)
		cc_data[ndx].Zero();
}

void NTV2_CC708CDPServiceInfoSection::Zero (void)
{
	ccsvcinfo_id = 0; svc_count = 0;
	svc_info_start = svc_info_change = svc_info_complete = false;
	for (int ndx(0);  ndx < 16;  ndx++)
		svc_info[ndx].Zero();
}
