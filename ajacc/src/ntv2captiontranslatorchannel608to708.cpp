/**
	@file		ntv2captiontranslatorchannel608to708.cpp
	@brief		Implementation of the CNTV2CaptionTranslatorChannel608to708 class.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc. All rights reserved.
**/


/*
		This module implements a translator for a single channel of CEA-608 ("Line 21") closed captioning.
	CEA-608 allows for up to eight "captioning channels" (CC1 - CC4, Text1 - Text4) that can be active
	and receiving data at a given time. Eight of these modules (one per CEA-608 caption channel) are
	instantiated by CNTV2CaptionTranslator608to708, which will parse the caption data stream and decide
	which channel to send the data to. This module should only be called with data that is pertinent to
	the channel it has been asked to translate.

	Note: this is a subclass of CNTV2CaptionDecodeChannel608, which does the major work of parsing and
	managing the state of the 608 captioning channel. This subclass overrides most of the "state change"
	calls to additionally provide translations to CEA-708 data. As such, the main high level method for
	this module is the parent class' Parse608Data(), which takes two bytes of data (the most any one 608
	channel can receive in a video frame) and decodes it. As the parent class makes calls to modify its
	state, this subclass overrides them and makes the necessary translations to 708.

		In addition to the CEA-608 caption state maintained by the CNTV2CaptionDecodeChannel608 class,
	this sub-class maintains the state of the various "windows" used by the 708 caption decoder. The
	state of these windows typically form the parameters of the output 708 commands.

		After calling Parse608Data() with each frame's 608 data, any translated commands are saved as
	CEA-708 "Service Blocks" in a FIFO queue. Users can call GetNextServiceBlockInfoFromQueue() to get
	the status of the queue and the its Service Block (if any). Service Blocks can be dequeued (one at
	a time) by calling GetNextServiceBlockFromQueue(). Note: this module packages each 708 command into
	its own self-contained Service Block. However, when putting together data for a 708 Caption Channel
	Packet, multiple commands targeted to the same Service Number can be put in a single Service Block.
	Because this code has no visibility of this higher-level packet "packaging", it is up to the user
	to concatenate separate Service Blocks from this module into a single combined Service Block when
	needed. To aid this effort, the method GetNextServiceBlockDataFromQueue() may be called to dequeue
	a Service Block but ONLY copy the Service Block's data payload.

	NOTE: the output Service Block queue is very simple and NOT thread-safe. If Parse608Data() is called
	from a different thread than the one dequeueing the output 708 Service Blocks you will need to add
	semaphores or some other thread-safe code. Also: there is no intelligence about queue overflows: once
	it's full all new 708 commands are dropped. It is up to the user to make sure that the queue is
	emptied (which shouldn't be a problem in 608->708 translation since the relative data rates are so
	different - 708 is much higher).

 */

#include <string.h>
#include "stdio.h"		// only needed for debug printf's
#include "ntv2captiontranslatorchannel608to708.h"
#include "ntv2debug.h"


using namespace std;


/////////////////////////////////////////////////////////////////////////////
// CaptionTranslator608to708 definition
/////////////////////////////////////////////////////////////////////////////
#if defined (MSWindows)
	#pragma warning(disable: 4800)
#endif


static unsigned	gInstanceTally	(0);


bool CNTV2CaptionTranslatorChannel608to708::Create (CNTV2CaptionTranslatorChannel608to708Ptr & outInstance)
{
	outInstance = NULL;

	try
	{
		outInstance = new CNTV2CaptionTranslatorChannel608to708;
	}
	catch (const std::bad_alloc &)
	{
	}
	return outInstance;

}	//	Create



/////////////////////////////////////////////////////////////////////////////
// Constructor
//
CNTV2CaptionTranslatorChannel608to708::CNTV2CaptionTranslatorChannel608to708 (void)
{
	gInstanceTally++;
	m_708ServiceNum = NTV2_CC708PrimaryCaptionServiceNum;

	m_608PopOn_OnScreenWindowID  = -1;
	m_608PopOn_OffScreenWindowID = -1;
	m_608RollUp_WindowID		 = -1;
	m_608PaintOn_WindowID		 = -1;
	m_608Text_WindowID			 = -1;

	m_lastUsedWindowID = 1;		//	This forces '0' to be the first one we use

	m_708ServiceNum = 0;		//	NOTE:	"0" is NOT a valid service number!

	if (!CNTV2CaptionEncoder708::Create (m_708Encoder))
	{
		std::bad_alloc	exception;
		throw exception;
	}
	Reset ();

}	//	constructor


CNTV2CaptionTranslatorChannel608to708::CNTV2CaptionTranslatorChannel608to708 (const CNTV2CaptionTranslatorChannel608to708 & inTranslatorChannelToCopy)
	:	CNTV2CaptionDecodeChannel608()
{
	gInstanceTally++;
	(void) inTranslatorChannelToCopy;
	AJACC_ASSERT (false);
}	//	copy constructor


CNTV2CaptionTranslatorChannel608to708::~CNTV2CaptionTranslatorChannel608to708 ()
{
}	//	destructor


CNTV2CaptionTranslatorChannel608to708 & CNTV2CaptionTranslatorChannel608to708::operator = (const CNTV2CaptionTranslatorChannel608to708 & inTranslatorChannelToCopy)
{
	(void) inTranslatorChannelToCopy;
	AJACC_ASSERT (false);
	return *this;
}	//	assignment operator


// Reset()
//
//	This can be called to "flush" the system of any in-progress data.
//	Note: this is NOT guaranteed to be thread-safe.
//
void CNTV2CaptionTranslatorChannel608to708::Reset (void)
{
	for (int i = 0;  i < NTV2_CC708NumWindows;  i++)
	{
		Init608CCWindowStatus (i);
		m_windowStatus [i].bDefined = false;
	}

	m_SvcBlockQueue.Flush ();
	m_708Encoder->Reset();

}	//	Reset


// Set708ServiceNumber()
//		Set the 708 Caption Service Number that this channel should use
//
void CNTV2CaptionTranslatorChannel608to708::Set708ServiceNumber (const int serviceNum)
{
	if (serviceNum >= 0 && serviceNum < 64)
		m_708ServiceNum = serviceNum;

}	//	Set708ServiceNumber


// Set708TranslateEnable()
//		Set the translate enable for this channel
//
void CNTV2CaptionTranslatorChannel608to708::Set708TranslateEnable(bool enable)
{
	m_708TranslateEnable = enable;
}


// SetChannel()
//		Set the decode channel ID that this instance is working on
//		-- override --
bool CNTV2CaptionTranslatorChannel608to708::SetChannel (const NTV2Line21Channel inChannel)
{
	if (!IsValidLine21Channel (inChannel))
		return false;	//	Bad channel value

	CNTV2CaptionDecodeChannel608::SetChannel (inChannel);
	m_SvcBlockQueue.SetDebugChannel (inChannel);
	return true;

}	//	SetChannel



//************************************************************************
//
//	CEA-708 Captioning "Window" default values
//
//************************************************************************

// Init608CCWindowStatus
//		Init a CCWindowStatus struct for default 608 values
//
bool CNTV2CaptionTranslatorChannel608to708::Init608CCWindowStatus (int winID, const NTV2Line21Mode inMode)
{
	if (winID < NTV2_CC708WindowIDMin || winID > NTV2_CC708WindowIDMax)
		return false;

	if (inMode <= NTV2_CC608_CapModeMin || inMode >= NTV2_CC608_CapModeMax)
		return false;

	CC708WindowStatus	status	(m_windowStatus [winID]);

	status.bDefined = true;
	status.bDirty   = false;

	Init608CCWindowParms		(m_windowStatus [winID].windowParms,	inMode);
	Init608CCWindowAttributes	(m_windowStatus [winID].windowAttr,		inMode);
	Init608CCPenAttributes		(m_windowStatus [winID].penAttr,		inMode);
	Init608CCPenColor			(m_windowStatus [winID].penColor,		inMode);
	Init608CCPenLocation		(m_windowStatus [winID].penLoc,			inMode);
	return true;

}	//	Init608CCWindowStatus


// Init608CCWindowParms
//		Init a CC708WindowParms struct for default 608 values
//
bool CNTV2CaptionTranslatorChannel608to708::Init608CCWindowParms (CC708WindowParms & outParms, const NTV2Line21Mode inMode) const
{
	if (inMode <= NTV2_CC608_CapModeMin || inMode >= NTV2_CC608_CapModeMax)
		return false;

	outParms.priority = NTV2_CC708Default608WindowPriority;

	//	PopOn, PaintOn, and Text modes anchor upper-left, RollOn is lower-left...
	if (!IsTextChannel ())
	{
		if (inMode == NTV2_CC608_CapModePopOn || inMode == NTV2_CC608_CapModePaintOn)
			outParms.anchorPt = NTV2_CC708WindowAnchorPointUpperLeft;
		else
			outParms.anchorPt = NTV2_CC708WindowAnchorPointLowerLeft;
	}
	else
		outParms.anchorPt = NTV2_CC708WindowAnchorPointUpperLeft;

	//	PopOn, PaintOn, and Text modes are absolute, RollUp is relative...
	if (!IsTextChannel ())
	{
		if (inMode == NTV2_CC608_CapModePopOn || inMode == NTV2_CC608_CapModePaintOn)
			outParms.relativePos	= NTV2_CC708AbsolutePos;
		else
			outParms.relativePos	= NTV2_CC708RelativePos;
	}
	else
		outParms.relativePos	= NTV2_CC708AbsolutePos;

	//	The anchor points and row/colCounts need to be calculated by the caller...
	outParms.anchorV		= 0;
	outParms.anchorH		= 0;
	if (!IsTextChannel ())
		outParms.rowCount	= 1;
	else
		outParms.rowCount	= NTV2_CC608_MaxRow;

	outParms.colCount		= NTV2_CC608_MaxCol;
	outParms.rowLock		= NTV2_CC708Lock;
	outParms.colLock		= NTV2_CC708Lock;
	outParms.visible		= NTV2_CC708NotVisible;		// note: it's up to the caller to make this window "visible" at the appropriate time

	//	Window/Pen styles depend on mode...
	if (!IsTextChannel ())
	{
		if (inMode == NTV2_CC608_CapModePopOn || inMode == NTV2_CC608_CapModePaintOn)
		{
			outParms.windowStyleID = NTV2_CC708Default608PopOnWindowStyleID;
			outParms.penStyleID	  = NTV2_CC708Default608PopOnPenStyleID;
		}
		else
		{
			outParms.windowStyleID = NTV2_CC708Default608RollUpWindowStyleID;
			outParms.penStyleID	  = NTV2_CC708Default608RollUpPenStyleID;
		}
	}
	else
	{
		outParms.windowStyleID = NTV2_CC708Default608TextWindowStyleID;
		outParms.penStyleID	  = NTV2_CC708Default608TextPenStyleID;
	}

	return true;

}	//	Init608CCWindowParms


// Init608CCWindowAttributes
//		Init a CC708WindowAttr struct for default 608 values
//
bool CNTV2CaptionTranslatorChannel608to708::Init608CCWindowAttributes (CC708WindowAttr & outAttr, const NTV2Line21Mode inMode) const
{
	(void) inMode;
	outAttr.justify			= NTV2_CC708Default608Justify;
	outAttr.printDir		= NTV2_CC708Default608PrintDir;
	outAttr.scrollDir		= NTV2_CC708Default608ScrollDir;
	outAttr.wordWrap		= NTV2_CC708Default608WordWrap;
	outAttr.displayEffect	= NTV2_CC708Default608DisplayEffect;
	outAttr.effectDir		= NTV2_CC708Default608EffectDir;

	if (!IsTextChannel ())
		outAttr.effectSpeed	= NTV2_CC708Default608EffectSpeed;
	else
		outAttr.effectSpeed	= NTV2_CC708Default608TextEffectSpeed;

	outAttr.fillColor	= NTV2_CC708BlackColor;				// n/a
	outAttr.borderType	= NTV2_CC708Default608BorderType;
	outAttr.borderColor	= NTV2_CC708BlackColor;				// n/a
	return true;

}	//	Init608CCWindowAttributes


// Init608CCPenAttributes
//		Init a CC708PenAttr struct for default 608 values
//
bool CNTV2CaptionTranslatorChannel608to708::Init608CCPenAttributes (CC708PenAttr & outAttr, const NTV2Line21Mode inMode) const
{
	(void) inMode;
	outAttr.penSize		= NTV2_CC708Default608PenSize;
	outAttr.fontStyle	= NTV2_CC708Default608FontStyle;
	outAttr.textTag		= NTV2_CC708Default608TextTag;
	outAttr.offset		= NTV2_CC708Default608PenOffset;
	outAttr.italics		= NTV2_CC708Default608Italics;
	outAttr.underline	= NTV2_CC708Default608Underline;
	outAttr.edgeType	= NTV2_CC708Default608PenEdgeType;
	return true;

}	//	Init608CCPenAttributes


// Init608CCPenColor
//		Init a CC708PenColor struct for default 608 values
//
bool CNTV2CaptionTranslatorChannel608to708::Init608CCPenColor (CC708PenColor & outColor, const NTV2Line21Mode inMode) const
{
	(void) inMode;
	outColor.fg   = NTV2_CC708WhiteColor;
	outColor.bg   = NTV2_CC708BlackColor;
	outColor.edge = NTV2_CC708WhiteColor;

	return true;

}	//	Init608CCPenColor


// Init608CCPenLocation()
//		Init a CC708PenLocation struct for default 608 values
//
bool CNTV2CaptionTranslatorChannel608to708::Init608CCPenLocation (CC708PenLocation & outLoc, const NTV2Line21Mode inMode) const
{
	(void) inMode;
	outLoc.row = outLoc.column = 0;
	return true;

}	//	Init608CCPenLocation


// DeleteWindow()
//		Change the CCWindowStatus struct at <index> to "delete" a window
//
bool CNTV2CaptionTranslatorChannel608to708::DeleteWindow (int winID)
{
	if (winID < NTV2_CC708WindowIDMin || winID > NTV2_CC708WindowIDMax)
		return false;

	m_windowStatus [winID].bDefined = false;
	m_windowStatus [winID].bDirty   = false;
	m_windowStatus [winID].windowParms.visible = false;

	return true;

}	//	DeleteWindow



// IsWindowDefined()
//		Returns 'true' if the designated window ID is within valid range AND it has been initialized
//
bool CNTV2CaptionTranslatorChannel608to708::IsWindowDefined (const int winID) const
{
	return winID >= NTV2_CC708WindowIDMin
			&& winID <= NTV2_CC708WindowIDMax
				&& m_windowStatus [winID].bDefined;

}	//	IsWindowDefined



// IsWindowDirty()
//		Returns 'true' if the designated window ID has had any content (i.e. characters) written to it
//
//	Note: this comes up because we sometimes see Line 21 captions with back-to-back PAC commands. If the 2nd PAC
//		  command is "above" the 1st, it will be ignored in our current window algorithm. However, if no content
//		  has been written to the window between the two PAC commands, it is more correct to ignore the first and
//		  redefine the window according to the second.
//
bool CNTV2CaptionTranslatorChannel608to708::IsWindowDirty (int winID) const
{
	bool bResult = true;

	if (winID >= NTV2_CC708WindowIDMin && winID <= NTV2_CC708WindowIDMax && m_windowStatus[winID].bDefined)
		bResult = m_windowStatus[winID].bDirty;

	return bResult;

}	//	IsWindowDirty



// GetCurrentEditWindowID()
//		Returns the current "edit" window, according to the designated mode (or the current mode if none provided).
//		If no window is currently defined, it creates one and returns a 'newWindow' flag. Returns '-1' if error.
//
bool CNTV2CaptionTranslatorChannel608to708::GetCurrentEditWindowID (int * pWindowID, NTV2Line21Mode mode, bool * pbNewWindow)
{
	bool	bResult		(true);
	int		winID		(-1);
	bool	bNewWindow	(false);	//	Set 'true' if we define a new window from scratch

	if (mode == NTV2_CC608_CapModeUnknown)
		mode = GetCaptionMode ();

	if (IsTextChannel ())
	{
		//	Check the current Text window - if not defined, make one...
		if (!IsWindowDefined (m_608Text_WindowID))
		{
				// init the official default Text window
			m_608Text_WindowID = NTV2_CC708DefaultTextWindowID;
			Init608CCWindowStatus (m_608Text_WindowID, NTV2_CC608_CapModePopOn);
			m_windowStatus [m_608Text_WindowID].windowParms.visible = true;
			bNewWindow = true;
		}

		winID = m_608Text_WindowID;
	}	//	if IsTextChannel
	else
	{
		switch (mode)
		{
			case NTV2_CC608_CapModePopOn:	//	Check the current PopUp off-screen window - if not defined, make one
				if (!IsWindowDefined (m_608PopOn_OffScreenWindowID))
				{
					//	Alternate windows between 0 and 1...
					int newWindowID = (m_lastUsedWindowID == NTV2_CC708DefaultPopOnWindowID ? NTV2_CC708DefaultPopOnWindowID+1 : NTV2_CC708DefaultPopOnWindowID);

					//	If the first choice is taken, try the other...
					if (m_windowStatus [newWindowID].bDefined && !m_windowStatus [m_lastUsedWindowID].bDefined)
						newWindowID = m_lastUsedWindowID;

					m_608PopOn_OffScreenWindowID = newWindowID;
					m_lastUsedWindowID = m_608PopOn_OffScreenWindowID;	//	Remember it so we can alternate next time

					Init608CCWindowStatus (m_608PopOn_OffScreenWindowID, mode);
					bNewWindow = true;
				}

				//	Offscreen PopOn windows should not be visible...
				m_windowStatus [m_608PopOn_OffScreenWindowID].windowParms.visible = false;

				winID = m_608PopOn_OffScreenWindowID;
				break;

			case NTV2_CC608_CapModePaintOn: 		//	Check the current PaintOn window - if not defined, make one
				if (!IsWindowDefined (m_608PaintOn_WindowID))
				{
					//	Init the official default RollUp window...
					m_608PaintOn_WindowID = NTV2_CC708DefaultPaintOnWindowID;
					Init608CCWindowStatus (m_608PaintOn_WindowID, mode);
					m_windowStatus [m_608PaintOn_WindowID].windowParms.visible = true;
					bNewWindow = true;
				}

				//	Paint-on windows are always visible...
				m_windowStatus[m_608PaintOn_WindowID].windowParms.visible = true;
				winID = m_608PaintOn_WindowID;
				break;

			case NTV2_CC608_CapModeRollUp2:
			case NTV2_CC608_CapModeRollUp3:
			case NTV2_CC608_CapModeRollUp4: 		//	Check the current RollUp window - if not defined, make one
				if (!IsWindowDefined (m_608RollUp_WindowID))
				{
					//	Init the official default RollUp window...
					m_608RollUp_WindowID = NTV2_CC708DefaultRollUpWindowID;
					Init608CCWindowStatus (m_608RollUp_WindowID, mode);
					m_windowStatus [m_608RollUp_WindowID].windowParms.visible = true;
					bNewWindow = true;
				}

				//	Roll-up windows are always visible...
				m_windowStatus [m_608RollUp_WindowID].windowParms.visible = true;
				winID = m_608RollUp_WindowID;
				break;

			default:
				break;
		}	//	switch on mode
	}	//	else not text channel

	//	Return the result...
	if (pWindowID)
		*pWindowID = winID;

	//	If the caller wants to know whether or not this is a new window, tell 'em!
	if (pbNewWindow)
		*pbNewWindow = bNewWindow;

	if (winID == -1)
	{
		Log () << "GetCurrentEditWindow - ERROR: can't find an edit window!" << endl;
		bResult = false;
	}

	return bResult;

}	//	GetCurrentEditWindowID


// GetCurrentModeWindowID()
//		Returns the current "edit" window, according to the designated mode (or the current mode if none provided).
//		If no window is currently defined for the given mode, this method returns "-1". (i.e. it does NOT create one!)
//
int CNTV2CaptionTranslatorChannel608to708::GetCurrentModeWindowID (void)
{
	int	winID	(-1);

	if (IsTextChannel ())
		winID = m_608Text_WindowID;
	else
	{
		switch (GetCaptionMode ())
		{
			case NTV2_CC608_CapModeRollUp2:
			case NTV2_CC608_CapModeRollUp3:
			case NTV2_CC608_CapModeRollUp4:	winID = m_608RollUp_WindowID;			break;

			case NTV2_CC608_CapModePopOn:	winID = m_608PopOn_OffScreenWindowID;	break;
			case NTV2_CC608_CapModePaintOn: winID = m_608PaintOn_WindowID;			break;
			default:																break;
		}
	}

	return winID;

}	//	GetCurrentModeWindowID


//***********************************************************************************************
//	CNTV2CaptionDecodeChannel608 parent class overrides
//***********************************************************************************************

// Parse608CharacterData()
//		Insert one or two characters.
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::Parse608CharacterData (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	UByte char708_1, char708_2;

	// call parent class to manage 608 state
	bool bResult = CNTV2CaptionDecodeChannel608::Parse608CharacterData (char608_1, char608_2, outDebugStr);

	// count how many non-zero characters we have
	int charCount = 0;
	if (char608_1 > 0)
	{
		charCount++;
		if (char608_2 > 0)
			charCount++;
	}

	// convert 1st character 608 code to 708
	Convert608CharacterTo708 (char608_1, char708_1, char708_2);
	if (charCount > 0)
	{
		if (char708_2 == 0)		// if this is a one-byte character...
			bResult = QueueServiceBlock_CharacterData (m_708ServiceNum, char708_1);
		else
			bResult = QueueServiceBlock_TwoByteData (m_708ServiceNum, char708_1, char708_2);
	}

	// convert 2nd character 608 code to 708
	Convert608CharacterTo708 (char608_2, char708_1, char708_2);
//	if (charCount > 1)
	if (charCount > 0)		// if we sent one character, send both...
	{
		if (char708_2 == 0)		// if this is a one-byte character...
			bResult = QueueServiceBlock_CharacterData (m_708ServiceNum, char708_1);
		else
			bResult = QueueServiceBlock_TwoByteData (m_708ServiceNum, char708_1, char708_2);
	}

	// the current window is now "dirty"
	int currWinID = GetCurrentModeWindowID ();
	if (IsWindowDefined (currWinID))
		m_windowStatus[currWinID].bDirty = true;

	return bResult;

}	//	Parse608CharacterData



// Parse608TabOffsetCommand()
//		Translate a 608 TabOffset command into equivalent 708 command.
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::Parse608TabOffsetCommand (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	// call parent class to manage 608 state
	bool bResult = CNTV2CaptionDecodeChannel608::Parse608TabOffsetCommand (char608_1, char608_2, outDebugStr);

	int currWinID = GetCurrentModeWindowID ();

	// no sense in sending the command if there is no current window (?)
	if (bResult && IsWindowDefined (currWinID))
	{
		// send an updated PenLocation
		GetCurrentPenLocation (m_windowStatus[currWinID].penLoc, currWinID);

		// Send a SetPenLocation command to report our new cursor position
		bResult = QueueServiceBlock_SetPenLocation (m_708ServiceNum, m_windowStatus[currWinID].penLoc);
	}

	return bResult;

}	//	Parse608TabOffsetCommand



// Parse608CharacterSetCommand()
//		Translate a 608 CharacterSet command into equivalent 708 command.
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::Parse608CharacterSetCommand (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	// call parent class to manage 608 state
	bool bResult = CNTV2CaptionDecodeChannel608::Parse608CharacterSetCommand (char608_1, char608_2, outDebugStr);

	// TBD: not sure what to do with this in 708-land...
	return bResult;

}	//	Parse608CharacterSetCommand



// Parse608AttributeCommand()
//		Translate a 608 Attribute command into equivalent 708 command.
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::Parse608AttributeCommand (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	// call parent class to manage 608 state
	bool bResult = CNTV2CaptionDecodeChannel608::Parse608AttributeCommand (char608_1, char608_2, outDebugStr);

	// TBD: not sure what to do with this in 708-land...
	return bResult;

}	//	Parse608AttributeCommand



// Parse608PACCommand()
//		Translate a 608 Preamble Address Code command into equivalent 708 command.
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::Parse608PACCommand (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	//	Call parent class to manage 608 state
	bool	bResult	(CNTV2CaptionDecodeChannel608::Parse608PACCommand (char608_1, char608_2, outDebugStr));
	int		PACRow	(GetRow ());
	//int	PACCol	(GetColumn ());
	if (!IsTextChannel ())
	{
		if (GetCaptionMode () == NTV2_CC608_CapModePopOn)
		{
			int rowCount;

			//	We have a "Preamble Address Code" (PAC) - this either means define a new window or redefine an existing window...
			bool bNewWindow = false;
			bool bGotWindow = GetCurrentEditWindowID (&m_608PopOn_OffScreenWindowID, NTV2_CC608_CapModePopOn, &bNewWindow);

			if (bGotWindow && IsWindowDefined (m_608PopOn_OffScreenWindowID))
			{
				if (bNewWindow)
				{
					//	This is a new window, not a re-defined one...
					rowCount = 1;			//	New windows always start with a height of one row
					m_windowStatus [m_608PopOn_OffScreenWindowID].windowParms.anchorV  = (PACRow - 1) * NTV2_CC608_TextRowHeight;	//	Assuming absolute coordinates, anchor upper-left
				}
				else if (!IsWindowDirty (m_608PopOn_OffScreenWindowID))
				{
					//	We already have an off-screen window defined, but haven't written anything to it yet -- so it's OK to redefine the window position...
					rowCount = m_windowStatus [m_608PopOn_OffScreenWindowID].windowParms.rowCount;
					m_windowStatus [m_608PopOn_OffScreenWindowID].windowParms.anchorV  = (PACRow - 1) * NTV2_CC608_TextRowHeight;	//	Assuming absolute coordinates, anchor upper-left

					//	If this is NOT a new window, change the penStyleID to '0' to indicate "no change" (just because...?)
					m_windowStatus [m_608PopOn_OffScreenWindowID].windowParms.penStyleID = 0;
				}
				else
				{
					//	We already have an off-screen window with content written to it, so we must be re-defining it...
					//	We need to calculate the new number of rows by subtracting the row number of the top of the window...
					int windowTopRow = (m_windowStatus [m_608PopOn_OffScreenWindowID].windowParms.anchorV / NTV2_CC608_TextRowHeight) + 1;
					rowCount = (PACRow - windowTopRow) + 1;

					//	Sanity check...
					if (rowCount < 1)
						rowCount = 1;

					//	Existing windows can't get smaller -- only larger...
					if (rowCount < m_windowStatus [m_608PopOn_OffScreenWindowID].windowParms.rowCount)
						rowCount = m_windowStatus [m_608PopOn_OffScreenWindowID].windowParms.rowCount;

					//	If this is NOT a new window, change the penStyleID to '0' to indicate "no change" (just because...?)
					m_windowStatus [m_608PopOn_OffScreenWindowID].windowParms.penStyleID = 0;
				}

				//	Update window status to new specs...
				m_windowStatus [m_608PopOn_OffScreenWindowID].windowParms.visible  = false;			//	The offscreen window should always be "not visible"
				m_windowStatus [m_608PopOn_OffScreenWindowID].windowParms.rowCount = rowCount;

				GetCurrentPenAttributes	(m_windowStatus [m_608PopOn_OffScreenWindowID].penAttr);
				GetCurrentPenColor		(m_windowStatus [m_608PopOn_OffScreenWindowID].penColor);
				GetCurrentPenLocation	(m_windowStatus [m_608PopOn_OffScreenWindowID].penLoc, m_608PopOn_OffScreenWindowID);

				//	Define Window...
				bResult = QueueServiceBlock_DefineWindow (m_708ServiceNum, m_608PopOn_OffScreenWindowID, m_windowStatus [m_608PopOn_OffScreenWindowID].windowParms);

				//	Update Pen Attributes/Color/Location...
				bResult = QueueServiceBlock_SetPenAttributes	(m_708ServiceNum, m_windowStatus [m_608PopOn_OffScreenWindowID].penAttr);
				bResult = QueueServiceBlock_SetPenColor			(m_708ServiceNum, m_windowStatus [m_608PopOn_OffScreenWindowID].penColor);
				bResult = QueueServiceBlock_SetPenLocation		(m_708ServiceNum, m_windowStatus [m_608PopOn_OffScreenWindowID].penLoc);
			}
			else
				Log () << "Parse608CCData - ERROR: Couldn't define new offscreen window - both windows are already active" << endl;
		}
		else if (IsLine21RollUpMode (GetCaptionMode ()))
		{
			//	How many rows?
			int rollUpRows = 2;
			switch (GetCaptionMode ())
			{
				default:
				case NTV2_CC608_CapModeRollUp2:	rollUpRows = 2;	break;
				case NTV2_CC608_CapModeRollUp3:	rollUpRows = 3;	break;
				case NTV2_CC608_CapModeRollUp4:	rollUpRows = 4;	break;
			}

			//	Make sure we have a defined window...
			bool	bGotWindow	(GetCurrentEditWindowID (&m_608RollUp_WindowID));

			if (bGotWindow && IsWindowDefined (m_608RollUp_WindowID))
			{
				//	Ad hoc:		Define the position of the bottom of the window, assuming relative positioning...
				if (PACRow < rollUpRows)
					PACRow = rollUpRows;
				int	anchorV	(((PACRow * 100) / NTV2_CC608_MaxRow) - 1);

				m_windowStatus [m_608RollUp_WindowID].windowParms.rowCount = rollUpRows;
				m_windowStatus [m_608RollUp_WindowID].windowParms.anchorV  = anchorV;

				GetCurrentPenAttributes	(m_windowStatus [m_608RollUp_WindowID].penAttr);
				GetCurrentPenColor		(m_windowStatus [m_608RollUp_WindowID].penColor);
				GetCurrentPenLocation	(m_windowStatus [m_608RollUp_WindowID].penLoc, m_608RollUp_WindowID);

				//	Define Window...
				bResult = QueueServiceBlock_DefineWindow (m_708ServiceNum, m_608RollUp_WindowID, m_windowStatus [m_608RollUp_WindowID].windowParms);

				//	Update Pen Attributes/Color/Location...
				bResult = QueueServiceBlock_SetPenAttributes	(m_708ServiceNum, m_windowStatus [m_608RollUp_WindowID].penAttr);
				bResult = QueueServiceBlock_SetPenColor			(m_708ServiceNum, m_windowStatus [m_608RollUp_WindowID].penColor);
				bResult = QueueServiceBlock_SetPenLocation		(m_708ServiceNum, m_windowStatus [m_608RollUp_WindowID].penLoc);
			}
		}
		else if (GetCaptionMode () == NTV2_CC608_CapModePaintOn)
		{
			//	We have a "Preamble Address Code" (PAC) - this either means define a new window or redefine an existing window...
			int		rowCount	(0);
			bool	bNewWindow	(false);
			bool	bGotWindow	(GetCurrentEditWindowID (&m_608PaintOn_WindowID, NTV2_CC608_CapModePaintOn, &bNewWindow));

			if (bGotWindow && IsWindowDefined (m_608PaintOn_WindowID))
			{
				if (bNewWindow)
				{
					//	This is a new window, not a re-defined one...
					rowCount = 1;			//	New windows always start with a height of one row
					m_windowStatus [m_608PaintOn_WindowID].windowParms.anchorV  = (PACRow - 1) * NTV2_CC608_TextRowHeight;	//	Assuming absolute coordinates, anchor upper-left
				}
				else if (!IsWindowDirty (m_608PaintOn_WindowID))
				{
					//	We already have a paint-on window defined but haven't written anything to it yet - so it's OK to redefine the window position...
					rowCount = m_windowStatus [m_608PaintOn_WindowID].windowParms.rowCount;
					m_windowStatus [m_608PaintOn_WindowID].windowParms.anchorV  = (PACRow - 1) * NTV2_CC608_TextRowHeight;	//	Assuming absolute coordinates, anchor upper-left

					//	If this is NOT a new window, change the penStyleID to '0' to indicate "no change" (just because...?)
					m_windowStatus [m_608PaintOn_WindowID].windowParms.penStyleID = 0;
				}
				else
				{
					//	We already have a window, so we must be re-defining it...

					//	We need to calculate the new number of rows by subtracting the row number of the top of the window...
					int	windowTopRow	((m_windowStatus[m_608PaintOn_WindowID].windowParms.anchorV / NTV2_CC608_TextRowHeight) + 1);
					rowCount = (PACRow - windowTopRow) + 1;

					//	Sanity check...
					if (rowCount < 1)
						rowCount = 1;

					//	Existing windows can't get smaller -- only larger...
					if (rowCount < m_windowStatus [m_608PaintOn_WindowID].windowParms.rowCount)
						rowCount = m_windowStatus [m_608PaintOn_WindowID].windowParms.rowCount;

					//	If this is NOT a new window, change the penStyleID to '0' to indicate "no change" (just because...?)
					m_windowStatus [m_608PaintOn_WindowID].windowParms.penStyleID = 0;
				}

				//	Update window status to new specs...
				m_windowStatus [m_608PaintOn_WindowID].windowParms.rowCount = rowCount;

				GetCurrentPenAttributes	(m_windowStatus [m_608PaintOn_WindowID].penAttr);
				GetCurrentPenColor		(m_windowStatus [m_608PaintOn_WindowID].penColor);
				GetCurrentPenLocation	(m_windowStatus [m_608PaintOn_WindowID].penLoc, m_608PaintOn_WindowID);

				//	Define Window...
				bResult = QueueServiceBlock_DefineWindow (m_708ServiceNum, m_608PaintOn_WindowID, m_windowStatus [m_608PaintOn_WindowID].windowParms);

				//	Update Pen Attributes/Color/Location...
				bResult = QueueServiceBlock_SetPenAttributes	(m_708ServiceNum, m_windowStatus [m_608PaintOn_WindowID].penAttr);
				bResult = QueueServiceBlock_SetPenColor			(m_708ServiceNum, m_windowStatus [m_608PaintOn_WindowID].penColor);
				bResult = QueueServiceBlock_SetPenLocation		(m_708ServiceNum, m_windowStatus [m_608PaintOn_WindowID].penLoc);
			}
			else
				Log () << "Parse608PACCommand - ERROR: Couldn't define new offscreen window - all windows are already active" << endl;
		}
	}
	else	//	IsTextChannel
	{
		bool	bNewWindow	(false);
		bool	bGotWindow	(GetCurrentEditWindowID (&m_608Text_WindowID, NTV2_CC608_CapModeUnknown, &bNewWindow));

		if (bGotWindow && IsWindowDefined (m_608Text_WindowID))
		{
			GetCurrentPenAttributes	(m_windowStatus [m_608Text_WindowID].penAttr);
			GetCurrentPenColor		(m_windowStatus [m_608Text_WindowID].penColor);
			GetCurrentPenLocation	(m_windowStatus [m_608Text_WindowID].penLoc, m_608Text_WindowID);

			bResult = QueueServiceBlock_SetCurrentWindow (m_708ServiceNum, m_608Text_WindowID);

			//	Update Pen Attributes/Color/Location...
			bResult = QueueServiceBlock_SetPenAttributes	(m_708ServiceNum, m_windowStatus [m_608Text_WindowID].penAttr);
			bResult = QueueServiceBlock_SetPenColor			(m_708ServiceNum, m_windowStatus [m_608Text_WindowID].penColor);
			bResult = QueueServiceBlock_SetPenLocation		(m_708ServiceNum, m_windowStatus [m_608Text_WindowID].penLoc);
		}
	}

	return bResult;

}	//	Parse608PACCommand


// Parse608MidRowCommand()
//		Translate a 608 Mid-Row Command into equivalent 708 command.
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::Parse608MidRowCommand (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	//	Call parent class to manage 608 state...
	bool	bResult		(CNTV2CaptionDecodeChannel608::Parse608MidRowCommand (char608_1, char608_2, outDebugStr));
	int		currWinID	(GetCurrentModeWindowID ());

	//	No sense in sending the command if there is no current window (?)
	if (bResult && IsWindowDefined (currWinID))
	{
		//	Mid-row codes occupy one "space"...
		bResult = QueueServiceBlock_CharacterData (m_708ServiceNum, ' ');

		//	Get the new pen color/attributes...
		GetCurrentPenColor		(m_windowStatus [currWinID].penColor);
		GetCurrentPenAttributes	(m_windowStatus [currWinID].penAttr);

		//	Update Pen Color/Attributes...
		bResult = QueueServiceBlock_SetPenColor			(m_708ServiceNum, m_windowStatus [currWinID].penColor);
		bResult = QueueServiceBlock_SetPenAttributes	(m_708ServiceNum, m_windowStatus [currWinID].penAttr);

		m_windowStatus [currWinID].bDirty = true;
	}

	return bResult;

}	//	Parse608MidRowCommand


// Parse608SpecialCharacter()
//		Translate a 608 2-Byte Character into equivalent 708 command.
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::Parse608SpecialCharacter (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	//	Call parent class to manage 608 state...
	bool	bResult	(CNTV2CaptionDecodeChannel608::Parse608SpecialCharacter (char608_1, char608_2, outDebugStr));
	UByte	char1 (0),	char2 (0);

	//	NOTE:	CNTV2CaptionDecodeChannel608 will translate these two-byte codes into ASCII characters
	//			we can use for our internal character generator. For 708, these need to also be translated
	//			into equivalent 708 codes.
	if (char608_1 == 0x11 || char608_1 == 0x19)
	{
		switch (char608_2)
		{
			case 0x30:	char1 = 0xae;					break;	// (R) "Registered Mark" symbol
			case 0x31:	char1 = 0xba;					break;	// Degree sign
			case 0x32:	char1 = 0xbd;					break;	// "1/2"
			case 0x33:	char1 = 0xbf;					break;	// Inverse query (upside-down question mark)
			case 0x34:	char1 = 0x10;	char2 = 0x39;	break;	// TM Trademark symbol
			case 0x35:	char1 = 0xa2;					break;	// Cents sign
			case 0x36:	char1 = 0xa3;					break;	// Pounds Sterling symbol
			case 0x37:	char1 = 0x7f;					break;	// Music note
			case 0x38:	char1 = 0xe0;					break;	// Lower-case a with grave accent
			case 0x39:	char1 = 0x10;	char2 = 0x20;	break;	// Transparent space
			case 0x3a:	char1 = 0xe8;					break;	// Lower-case e with grave accent
			case 0x3b:	char1 = 0xe2;					break;	// Lower-case a with circumflex
			case 0x3c:	char1 = 0xea;					break;	// Lower-case e with circumflex
			case 0x3d:	char1 = 0xee;					break;	// Lower-case i with circumflex
			case 0x3e:	char1 = 0xf4;					break;	// Lower-case o with circumflex
			case 0x3f:	char1 = 0xfb;					break;	// Lower-case u with circumflex

			default:									break;
		}
	}
	else if (char608_1 == 0x12 || char608_1 == 0x1a)
	{
		switch (char608_2)
		{
			case 0x20:	char1 = 0xc1;					break;	// Upper-case A with acute accent
			case 0x21:	char1 = 0xc9;					break;	// Upper-case E with acute accent
			case 0x22:	char1 = 0xd3;					break;	// Upper-case O with acute accent
			case 0x23:	char1 = 0xda;					break;	// Upper-case U with acute accent
			case 0x24:	char1 = 0xdc;					break;	// Upper-case U with umlaut
			case 0x25:	char1 = 0xfc;					break;	// Lower-case u with umlaut
			case 0x26:	char1 = 0x10;	char2 = 0x31;	break;	// Opening single quote
			case 0x27:	char1 = 0xa1;					break;	// Inverted exclamation mark
			case 0x28:	char1 = 0x2a;					break;	// Asterisk
			case 0x29:	char1 = 0x27;					break;	// Single quote
			case 0x2a:	char1 = 0x10;	char2 = 0x7d;	break;	// Em dash
			case 0x2b:	char1 = 0xa9;					break;	// Copyright
			case 0x2c:	char1 = 0x10;	char2 = 0x3d;	break;	// Service mark
			case 0x2d:	char1 = 0x10;	char2 = 0x35;	break;	// Round bullet
			case 0x2e:	char1 = 0x10;	char2 = 0x33;	break;	// Opening double quotes
			case 0x2f:	char1 = 0x10;	char2 = 0x34;	break;	// Closing double quotes

			case 0x30:	char1 = 0xc0;					break;	// Upper-case A with grave accent
			case 0x31:	char1 = 0xc2;					break;	// Upper-case A with circumflex accent
			case 0x32:	char1 = 0xc7;					break;	// Upper-case C with cedilla
			case 0x33:	char1 = 0xc8;					break;	// Upper-case E with grave accent
			case 0x34:	char1 = 0xca;					break;	// Upper-case E with circumflex accent
			case 0x35:	char1 = 0xcb;					break;	// Upper-case E with umlaut
			case 0x36:	char1 = 0xeb;					break;	// Lower-case e with umlaut
			case 0x37:	char1 = 0xce;					break;	// Upper-case I with circumflex accent
			case 0x38:	char1 = 0xcf;					break;	// Upper-case I with umlaut
			case 0x39:	char1 = 0xef;					break;	// Lower-case I with umlaut
			case 0x3a:	char1 = 0xd4;					break;	// Upper-case O with circumflex accent
			case 0x3b:	char1 = 0xd9;					break;	// Upper-case U with grave accent
			case 0x3c:	char1 = 0xf9;					break;	// Lower-case u with grave accent
			case 0x3d:	char1 = 0xdb;					break;	// Upper-case U with circumflex accent
			case 0x3e:	char1 = 0xab;					break;	// Opening guillemets
			case 0x3f:	char1 = 0xbb;					break;	// Closing guillemets

			default:									break;
		}
	}
	else if (char608_1 == 0x13 || char608_1 == 0x1b)
	{
		switch (char608_2)
		{
			case 0x20:	char1 = 0xc3;					break;	// Upper-case A with tilde
			case 0x21:	char1 = 0xe3;					break;	// Lower-case a with tilde
			case 0x22:	char1 = 0xcd;					break;	// Upper-case I with acute accent
			case 0x23:	char1 = 0xcc;					break;	// Upper-case I with grave accent
			case 0x24:	char1 = 0xec;					break;	// Lower-case i with grave accent
			case 0x25:	char1 = 0xd2;					break;	// Upper-case O with grave accent
			case 0x26:	char1 = 0xf2;					break;	// Lower-case o with grave accent
			case 0x27:	char1 = 0xd5;					break;	// Upper-case O with tilde
			case 0x28:	char1 = 0xf5;					break;	// Lower-case o with tilde
			case 0x29:	char1 = 0x7b;					break;	// Opening brace
			case 0x2a:	char1 = 0x7d;					break;	// Closing brace
			case 0x2b:	char1 = 0x5c;					break;	// Backslash
			case 0x2c:	char1 = 0x5e;					break;	// Caret
			case 0x2d:	char1 = 0x5f;					break;	// Underbar
			case 0x2e:	char1 = 0x7c;					break;	// Pipe
			case 0x2f:	char1 = 0x7e;					break;	// Tilde

			case 0x30:	char1 = 0xc4;					break;	// Upper-case A with umlaut
			case 0x31:	char1 = 0xe4;					break;	// Lower-case a with umlaut
			case 0x32:	char1 = 0xd6;					break;	// Upper-case O with umlaut
			case 0x33:	char1 = 0xf6;					break;	// Lower-case o with umlaut
			case 0x34:	char1 = 0xdf;					break;	// Small sharp s
			case 0x35:	char1 = 0xa5;					break;	// Yen
			case 0x36:	char1 = 0xa4;					break;	// Non-specific currency sign
			case 0x37:	char1 = 0x10;	char2 = 0x7a;	break;	// Vertical bar
			case 0x38:	char1 = 0xc5;					break;	// Upper-case A with ring
			case 0x39:	char1 = 0xe5;					break;	// Lower-case a with ring
			case 0x3a:	char1 = 0xd8;					break;	// Upper-case O with slash
			case 0x3b:	char1 = 0xf8;					break;	// Lower-case o with slash
			case 0x3c:	char1 = 0x10;	char2 = 0x7f;	break;	// Upper-left corner
			case 0x3d:	char1 = 0x10;	char2 = 0x7b;	break;	// Upper-right corner
			case 0x3e:	char1 = 0x10;	char2 = 0x7c;	break;	// Lower-left corner
			case 0x3f:	char1 = 0x10;	char2 = 0x7e;	break;	// Lower-right corner

			default:									break;
		}
	}

	if (char2 == 0)		//	If this is a one-byte character...
		bResult = QueueServiceBlock_CharacterData (m_708ServiceNum, char1);
	else
		bResult = QueueServiceBlock_TwoByteData (m_708ServiceNum, char1, char2);

	//	The current window is now "dirty"...
	const int	currWinID	(GetCurrentModeWindowID ());
	if (IsWindowDefined (currWinID))
		m_windowStatus [currWinID].bDirty = true;

	return bResult;

}	//	Parse608SpecialCharacter


// Parse608MiscCommand()
//		No override needed - the work is done by the separate "DoXXX()" misc command handlers (below)


//******************************************************************************
//
//	Specific handlers for 608 "Miscellaneous" commands
//
//******************************************************************************


// DoResumeCaptionLoading()
//		No override needed...


// DoBackspace()
//		Handles a 608 Backspace [BS] command
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::DoBackspace (void)
{
	//	Call parent class to manage 608 state...
	bool	bResult	(CNTV2CaptionDecodeChannel608::DoBackspace ());

	//	Now translate to appropriate 708 commands...

	//	A 708 Backspace command is a single character (0x08) which is just sent by iteself --
	//	so we can just pretend it's any old ASCII character...
	bResult = QueueServiceBlock_CharacterData (m_708ServiceNum, 0x08);

	return bResult;

}	//	DoBackspace


// DoDeleteToEndOfRow()
//		Handles a 608 Delete to End of Row [DER] command
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::DoDeleteToEndOfRow (void)
{
	//	Call parent class to manage 608 state...
	bool	bResult	(CNTV2CaptionDecodeChannel608::DoDeleteToEndOfRow ());

	//	Now translate to appropriate 708 commands...

	int		winID		(-1);
	bool	bGotWindow	(GetCurrentEditWindowID (&winID));
	if (bGotWindow && IsWindowDefined (winID))
	{
		//	There is no single 708 command that is equivalent to "Delete to End of Row", so we're
		//	going to transmit "transparent space" characters to cover any characters between the
		//	current column position and the end of the row. Then we'll reposition the cursor
		//	back to the original column...
		int	curr608Column	(GetColumn ());
		int	numColumnsToEnd	(NTV2_CC608_MaxCol - curr608Column + 1);

		for (int col = 0; col < numColumnsToEnd; col++)
		{
			//	"Transparent Space" is a two byte command: [EXT1] followed by [TSP]
			QueueServiceBlock_TwoByteData (m_708ServiceNum, 0x10, 0x20);	//	[EXT1], [TSP]
		}

		//	Send a 708 SetPenLocation command to bring cursor back to original spot...
		GetCurrentPenLocation (m_windowStatus [winID].penLoc, winID);
		bResult = QueueServiceBlock_SetPenLocation (m_708ServiceNum, m_windowStatus [winID].penLoc);
	}

	return bResult;

}	//	DoDeleteToEndOfRow


// DoRollUpCaption()
//		Handles 608 RollUpCaption [RU2], [RU3], [RU4] commands
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::DoRollUpCaption (const UWord rows)
{
	//	Call parent class to manage 608 state...
	bool	bResult	(CNTV2CaptionDecodeChannel608::DoRollUpCaption (rows));

	//	Translate to appropriate 708 commands...

	UWord	rowCount	(rows);

	if (!IsTextChannel ())
	{
		bool	bGotWindow	(GetCurrentEditWindowID (&m_608RollUp_WindowID));
		if (bGotWindow && IsWindowDefined (m_608RollUp_WindowID))
		{
			m_windowStatus [m_608RollUp_WindowID].windowParms.rowCount = rowCount;

			GetCurrentPenAttributes	(m_windowStatus [m_608RollUp_WindowID].penAttr);
			GetCurrentPenColor		(m_windowStatus [m_608RollUp_WindowID].penColor);
			GetCurrentPenLocation	(m_windowStatus [m_608RollUp_WindowID].penLoc, m_608RollUp_WindowID);

			//	Define Window...
			bResult = QueueServiceBlock_DefineWindow (m_708ServiceNum, m_608RollUp_WindowID, m_windowStatus [m_608RollUp_WindowID].windowParms);

			//	Update Pen Attributes/Color/Location...
			bResult = QueueServiceBlock_SetPenAttributes	(m_708ServiceNum, m_windowStatus [m_608RollUp_WindowID].penAttr);
			bResult = QueueServiceBlock_SetPenColor			(m_708ServiceNum, m_windowStatus [m_608RollUp_WindowID].penColor);
			bResult = QueueServiceBlock_SetPenLocation		(m_708ServiceNum, m_windowStatus [m_608RollUp_WindowID].penLoc);
		}
	}

	return bResult;

}	//	DoRollUpCaption


// DoFlashOn()
//		Handles a 608 Flash On [FON] command
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::DoFlashOn(void)
{
	//	Call parent class to manage 608 state...
	bool		bResult		(CNTV2CaptionDecodeChannel608::DoFlashOn ());
	const int	currWinID	(GetCurrentModeWindowID ());

	if (bResult && IsWindowDefined (currWinID))
	{
		GetCurrentPenColor (m_windowStatus [currWinID].penColor);

		//	By 608 caption rules, "Flash On" commands also move the cursor one space to the right --
		//	which acts just like a "space" character. To do that in 708-speak, we simply insert
		//	a space character here...
		bResult = QueueServiceBlock_CharacterData (m_708ServiceNum, 0x20);

		//	The "Flash" attribute is carried in the foreground pen color opacity...
		bResult = QueueServiceBlock_SetPenColor (m_708ServiceNum, m_windowStatus [currWinID].penColor);
	}

	return bResult;

}	//	DoFlashOn


// DoResumeDirectCaptioning()
//		No override needed...


// DoTextRestart()
//		Handles a Text Restart [TRS] command
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::DoTextRestart (void)
{
	bool	bResult	(CNTV2CaptionDecodeChannel608::DoTextRestart ());

	if (IsTextChannel ())
	{
		const bool	bGotWindow	(GetCurrentEditWindowID (&m_608Text_WindowID));
		if (bGotWindow && IsWindowDefined (m_608Text_WindowID))
		{
			//	Delete all (other) windows...
			UByte	windowMask	(1 << m_608Text_WindowID);
			bResult = QueueServiceBlock_DeleteWindows (m_708ServiceNum, ~(windowMask));

			GetCurrentPenAttributes	(m_windowStatus [m_608Text_WindowID].penAttr);
			GetCurrentPenColor		(m_windowStatus [m_608Text_WindowID].penColor);

			//	Clear Text Window...
			bResult = QueueServiceBlock_ClearWindows (m_708ServiceNum, windowMask);

			//	Define Window
			bResult = QueueServiceBlock_DefineWindow (m_708ServiceNum, m_608Text_WindowID, m_windowStatus [m_608Text_WindowID].windowParms);

			//	Update Window Attributes...
			bResult = QueueServiceBlock_SetWindowAttributes (m_708ServiceNum, m_windowStatus [m_608Text_WindowID].windowAttr);

			//	Update Pen Attributes/Color/Location...
			bResult = QueueServiceBlock_SetPenAttributes	(m_708ServiceNum, m_windowStatus [m_608Text_WindowID].penAttr);
			bResult = QueueServiceBlock_SetPenColor			(m_708ServiceNum, m_windowStatus [m_608Text_WindowID].penColor);
		}
	}

	return bResult;

}	//	DoTextRestart


// DoResumeTextDisplay()
//		Handles a Resume Text Display [RTD] command
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::DoResumeTextDisplay (void)
{
	bool	bResult	(CNTV2CaptionDecodeChannel608::DoResumeTextDisplay ());

	if (IsTextChannel ())
	{
		const bool	bGotWindow	(GetCurrentEditWindowID (&m_608Text_WindowID));
		if (bGotWindow && IsWindowDefined (m_608Text_WindowID))
		{
			//	Delete all (other) windows...
			UByte	windowMask	(~(1 << m_608Text_WindowID));
			bResult = QueueServiceBlock_DeleteWindows (m_708ServiceNum, windowMask);

			GetCurrentPenAttributes	(m_windowStatus [m_608Text_WindowID].penAttr);
			GetCurrentPenColor		(m_windowStatus [m_608Text_WindowID].penColor);

			//	Define Window...
			bResult = QueueServiceBlock_DefineWindow (m_708ServiceNum, m_608Text_WindowID, m_windowStatus [m_608Text_WindowID].windowParms);

			//	Update Window Attributes...
			bResult = QueueServiceBlock_SetWindowAttributes (m_708ServiceNum, m_windowStatus [m_608Text_WindowID].windowAttr);

			//	Update Pen Attributes/Color/Location...
			bResult = QueueServiceBlock_SetPenAttributes	(m_708ServiceNum, m_windowStatus [m_608Text_WindowID].penAttr);
			bResult = QueueServiceBlock_SetPenColor			(m_708ServiceNum, m_windowStatus [m_608Text_WindowID].penColor);
		}
	}

	return bResult;

}	//	DoResumeTextDisplay


// DoEraseDisplayedMemory()
//		Handles a 608 EraseDisplayedMemory [EDM] command
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::DoEraseDisplayedMemory (void)
{
	//	Call parent class to manage 608 state...
	bool	bResult	(CNTV2CaptionDecodeChannel608::DoEraseDisplayedMemory ());

	//	Now translate to appropriate 708 commands...

	//	In theory, this shouldn't be called for Text channels...
	if (!IsTextChannel ())
	{
		//	Make the window ID bitmap for all windows that need erasing...
		UByte	windowMap	(0);

		if (IsWindowDefined (m_608PopOn_OnScreenWindowID))
		{
			windowMap |= (1 << m_608PopOn_OnScreenWindowID);
			DeleteWindow (m_608PopOn_OnScreenWindowID);
			m_608PopOn_OnScreenWindowID = -1;
		}

		if ( IsWindowDefined (m_608RollUp_WindowID))
		{
			windowMap |= (1 << m_608RollUp_WindowID);
			DeleteWindow (m_608RollUp_WindowID);
			m_608RollUp_WindowID = -1;
		}

		if (IsWindowDefined (m_608PaintOn_WindowID))
		{
			windowMap |= (1 << m_608PaintOn_WindowID);
			DeleteWindow (m_608PaintOn_WindowID);
			m_608PaintOn_WindowID = -1;
		}

		if (windowMap)
		{
			//	Send a 708 DeleteWindows command...
			bResult = QueueServiceBlock_DeleteWindows (m_708ServiceNum, windowMap);
		}

		//	This is not strictly necessary, but it makes us match our target model...
		if (GetCaptionMode () == NTV2_CC608_CapModePopOn)
			m_lastUsedWindowID = 1;		//	Force the next DefineWindow to start with '0'
	}

	return bResult;

}	//	DoEraseDisplayedMemory


// DoCarriageReturn()
//		Handles a 608 Carriage Return [CR] command
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::DoCarriageReturn (void)
{
	//	Call parent class to manage 608 state...
	bool	bResult	(CNTV2CaptionDecodeChannel608::DoCarriageReturn ());

	if (!IsTextChannel ())
	{
		if (IsLine21RollUpMode (GetCaptionMode ()) && IsWindowDefined (m_608RollUp_WindowID))
		{
			GetCurrentPenColor		(m_windowStatus [m_608RollUp_WindowID].penColor);
			GetCurrentPenAttributes	(m_windowStatus [m_608RollUp_WindowID].penAttr);

			//	Send a 708 "Carriage Return" code...
			bResult = QueueServiceBlock_CharacterData (m_708ServiceNum, 0x0d);

			//	Update the Pen Attributes and Color...
			bResult = QueueServiceBlock_SetPenAttributes	(m_708ServiceNum, m_windowStatus [m_608RollUp_WindowID].penAttr);
			bResult = QueueServiceBlock_SetPenColor			(m_708ServiceNum, m_windowStatus [m_608RollUp_WindowID].penColor);
		}
	}
	else
	{
		if (IsWindowDefined (m_608Text_WindowID))
		{
			GetCurrentPenColor		(m_windowStatus [m_608Text_WindowID].penColor);
			GetCurrentPenAttributes	(m_windowStatus [m_608Text_WindowID].penAttr);
			GetCurrentPenLocation	(m_windowStatus [m_608Text_WindowID].penLoc, m_608Text_WindowID);

			//	Send a 708 "Carriage Return" code...
			bResult = QueueServiceBlock_CharacterData (m_708ServiceNum, 0x0d);

			//	Update the Pen Attributes, Color, and Location...
			bResult = QueueServiceBlock_SetPenAttributes	(m_708ServiceNum, m_windowStatus [m_608Text_WindowID].penAttr);
			bResult = QueueServiceBlock_SetPenColor			(m_708ServiceNum, m_windowStatus [m_608Text_WindowID].penColor);
			bResult = QueueServiceBlock_SetPenLocation		(m_708ServiceNum, m_windowStatus [m_608Text_WindowID].penLoc);
		}
	}

	return bResult;

}	//	DoCarriageReturn


// DoEraseNonDisplayedMemory()
//		Handles a 608 EraseNonDisplayedMemory [ENM] command
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::DoEraseNonDisplayedMemory (void)
{
	//	Call parent class to manage 608 state...
	bool	bResult	(CNTV2CaptionDecodeChannel608::DoEraseNonDisplayedMemory ());

	//	Now translate to appropriate 708 commands...

	//	In theory, this shouldn't be called for Text channels...
	if (!IsTextChannel ())
	{
		//	Make the window ID bitmap for all windows EXCEPT the current on-air one(s)...
		UByte	windowMap	(0);
		for (int i = NTV2_CC708WindowIDMin; i <= NTV2_CC708WindowIDMax; i++)
		{
			if (!m_windowStatus [i].windowParms.visible)
			{
				windowMap |= (1 << i);

				//	If the window is defined, delete it...
				if (m_windowStatus [i].bDefined)
				{
					DeleteWindow (i);

					//	If a deleted window was being used by a mode, clear it...
					if (m_608PopOn_OnScreenWindowID == i)
						m_608PopOn_OnScreenWindowID = -1;

					if (m_608PopOn_OffScreenWindowID == i)
						m_608PopOn_OffScreenWindowID = -1;

					if (m_608RollUp_WindowID == i)
						m_608RollUp_WindowID = -1;

					if (m_608PaintOn_WindowID == i)
						m_608PaintOn_WindowID = -1;
				}
			}
		}

		//	Send a 708 DeleteWindows command...
		bResult = QueueServiceBlock_DeleteWindows (m_708ServiceNum, windowMap);
	}

	return bResult;

}	//	DoEraseNonDisplayedMemory


// DoEndOfCaption()
//		Handles a 608 EndOfCaption [EOC] command
//		NOTE: this is an override of the CNTV2CaptionDecodeChannel608 parent class method,
//		and must call the parent class to maintain the current 608 state.
//
bool CNTV2CaptionTranslatorChannel608to708::DoEndOfCaption (void)
{
	//	Call parent class to manage 608 state...
	bool	bResult	(CNTV2CaptionDecodeChannel608::DoEndOfCaption ());

	//	End of Caption [EOC] - flip memories...
	if (!IsTextChannel () && GetCaptionMode () == NTV2_CC608_CapModePopOn)
	{
		//	Delete the current on-air window...
		if (IsWindowDefined (m_608PopOn_OnScreenWindowID))
		{
#if 1
			//	NOTE:	Technically, we're not supposed to erase/delete the out-going window -- just take it off-air
			//			and wait for the author to erase it. But practically speaking, we're seeing a lot of cases
			//			where the offscreen window is left "hanging", and (wrongly) reappears on the air later...
			bResult = QueueServiceBlock_DeleteWindows (m_708ServiceNum, (1 << m_608PopOn_OnScreenWindowID));
			DeleteWindow (m_608PopOn_OnScreenWindowID);
			m_608PopOn_OnScreenWindowID = -1;
#else
			bResult = QueueServiceBlock_HideWindows (m_708ServiceNum, (1 << m_608PopOn_OnScreenWindowID));
			m_windowStatus [m_608PopOn_OnScreenWindowID].windowParms.visible = false;
#endif
		}

		//	Show the current off-air window...
		if (IsWindowDefined (m_608PopOn_OffScreenWindowID))
		{
			bResult = QueueServiceBlock_DisplayWindows (m_708ServiceNum, (1 << m_608PopOn_OffScreenWindowID));
			m_windowStatus [m_608PopOn_OffScreenWindowID].windowParms.visible = true;

			//	Swap off-screen and on-screen windows...
			const int	temp	(m_608PopOn_OnScreenWindowID);
			m_608PopOn_OnScreenWindowID  = m_608PopOn_OffScreenWindowID;
			m_608PopOn_OffScreenWindowID = temp;
		}
	}

	return bResult;

}	//	DoEndOfCaption


// GetCurrentPenLocation()
//		Translate 608 current cursor location into a 708 Pen Location struct
//
bool CNTV2CaptionTranslatorChannel608to708::GetCurrentPenLocation (CC708PenLocation & outLoc, int windowID) const
{
	//	NOTE:	608 cursor position is "1-based" (i.e. 1-32 and 1-15). 708 is "0-based".
	outLoc.row	 = GetRow () - 1;
	outLoc.column = GetColumn () - 1;

	//	If we're handed a WindowID, it means we need to compensate for the window offset
	//	(i.e. a 708 window doesn't necessarily "start" at the top of the screen)...
	if (!IsWindowDefined (windowID))
	{
		int windowRowOffset = GetWindowRowOffset (windowID);

		if (windowRowOffset <= outLoc.row)
			outLoc.row -= windowRowOffset;
		else
			outLoc.row = 0;	//	I think this is wrong: this happens when a window can't be grown in the "up" direction,
							//	so the new row "location" is clipped to the existing top of the window
	}

	return true;

}	//	GetCurrentPenLocation


// GetWindowRowOffset()
//		If a 708-style window does not fill the screen (or start at the top of the screen), then the row position
//	within the window will not match the "absolute" (screen-oriented) row positions of 608. This routine looks at
//	the position of the window on the screen and returns an offset that can be used to translate from screen rows
//	to window rows: e.g.  windowRow = screenRow - GetWindowRowOffset(winID);
//
UWord CNTV2CaptionTranslatorChannel608to708::GetWindowRowOffset (int windowID) const
{
	UWord	result	(0);

	if (windowID >= NTV2_CC708WindowIDMin && windowID <= NTV2_CC708WindowIDMax)
	{
		switch (m_windowStatus [windowID].windowParms.anchorPt)
		{
			case NTV2_CC708WindowAnchorPointUpperLeft:
			case NTV2_CC708WindowAnchorPointUpperMiddle:
			case NTV2_CC708WindowAnchorPointUpperRight:
					if (m_windowStatus [windowID].windowParms.relativePos)
						result = (UWord(m_windowStatus[windowID].windowParms.anchorV) * NTV2_CC608_MaxRow) / 100;
					else
						result = UWord(m_windowStatus[windowID].windowParms.anchorV) / NTV2_CC608_TextRowHeight;			// <- typical forPaintOn/PopOn modes
					break;

			case NTV2_CC708WindowAnchorPointCenterLeft:
			case NTV2_CC708WindowAnchorPointCenterMiddle:
			case NTV2_CC708WindowAnchorPointCenterRight:
					if (m_windowStatus [windowID].windowParms.relativePos)
						result = ((UWord(m_windowStatus[windowID].windowParms.anchorV) * NTV2_CC608_MaxRow) / 100) - (UWord(m_windowStatus[windowID].windowParms.rowCount - 1) / 2);
					else
						result =  (UWord(m_windowStatus[windowID].windowParms.anchorV) / NTV2_CC608_TextRowHeight) - (UWord(m_windowStatus[windowID].windowParms.rowCount - 1) / 2);
					break;

			case NTV2_CC708WindowAnchorPointLowerLeft:
			case NTV2_CC708WindowAnchorPointLowerMiddle:
			case NTV2_CC708WindowAnchorPointLowerRight:
					if (m_windowStatus [windowID].windowParms.relativePos)
						result = ((UWord(m_windowStatus[windowID].windowParms.anchorV) * NTV2_CC608_MaxRow) / 100) - UWord(m_windowStatus[windowID].windowParms.rowCount - 1); // <- typical for RollUp modes
					else
						result =  (UWord(m_windowStatus[windowID].windowParms.anchorV) / NTV2_CC608_TextRowHeight) - UWord(m_windowStatus[windowID].windowParms.rowCount - 1);
					break;
		}
	}

	//	Reality check...
	if (result > (NTV2_CC608_MaxRow - 1))
		result = NTV2_CC608_MaxRow - 1;

	return result;

}	//	GetWindowRowOffset


// GetCurrentPenColor()
//		Translate 608 current attributes into a 708 Pen Color struct
//
bool CNTV2CaptionTranslatorChannel608to708::GetCurrentPenColor (CC708PenColor & outColor) const
{
	//	Get the attributes for the current location...
	UWord	screen	(GetCurrentScreen ());
	if (IsLine21PopOnMode (GetCaptionMode ()))		//	In PopUp mode, get attributes from offscreen array
		screen = (GetCurrentScreen () == 0) ? 1 : 0;

	NTV2Line21Attributes	attr;
	GetCharacterAttributes (screen, GetRow (), GetColumn (), attr);

	if (!attr.IsSet ())
		Init608CCPenColor (outColor, GetCaptionMode ());		//	If no attributes are set, load the 608 defaults
	else
	{
		//	Translate 608 foreground color to 708...
		switch (attr.GetColor ())
		{
			case NTV2_CC608_White:		outColor.fg = NTV2_CC708WhiteColor;		break;
			case NTV2_CC608_Green:		outColor.fg = NTV2_CC708GreenColor;		break;
			case NTV2_CC608_Blue:		outColor.fg = NTV2_CC708BlueColor;		break;
			case NTV2_CC608_Cyan:		outColor.fg = NTV2_CC708CyanColor;		break;
			case NTV2_CC608_Red:		outColor.fg = NTV2_CC708RedColor;		break;
			case NTV2_CC608_Yellow:		outColor.fg = NTV2_CC708YellowColor;	break;
			case NTV2_CC608_Magenta:	outColor.fg = NTV2_CC708MagentaColor;	break;
			case NTV2_CC608_Black:		outColor.fg = NTV2_CC708BlackColor;		break;
			case NTV2_CC608_NumColors:											break;
		}

		//	In 708-speak, the 608 "Flash" attribute is carried in the foreground opacity...
		if (attr.IsFlashing ())
			outColor.fg.opacity = NTV2_CC708OpacityFlash;

		//	Translate 608 background color to 708 (assume background is opaque)...
		switch (attr.GetBGColor ())
		{
			case NTV2_CC608_White:		outColor.bg = NTV2_CC708WhiteColor;		break;
			case NTV2_CC608_Green:		outColor.bg = NTV2_CC708GreenColor;		break;
			case NTV2_CC608_Blue:		outColor.bg = NTV2_CC708BlueColor;		break;
			case NTV2_CC608_Cyan:		outColor.bg = NTV2_CC708CyanColor;		break;
			case NTV2_CC608_Red:		outColor.bg = NTV2_CC708RedColor;		break;
			case NTV2_CC608_Yellow:		outColor.bg = NTV2_CC708YellowColor;	break;
			case NTV2_CC608_Magenta:	outColor.bg = NTV2_CC708MagentaColor;	break;
			case NTV2_CC608_Black:		outColor.bg = NTV2_CC708BlackColor;		break;
			case NTV2_CC608_NumColors:											break;
		}

		//	Tweak the background opacity as needed...
		if (attr.GetOpacity () == NTV2_CC608_SemiTransparent)
			outColor.bg.opacity = NTV2_CC708OpacityTranslucent;
		else if (attr.GetOpacity () == NTV2_CC608_Transparent)
			outColor.bg.opacity = NTV2_CC708OpacityTransparent;

		//	Assume that the edge color is the same as the foreground (??)...
		outColor.edge = outColor.fg;

		//	Assume that the edge color is White...
		//outColor.edge = NTV2_CC708WhiteColor;
	}
	return true;

}	//	GetCurrentPenColor


// GetCurrentPenAttributes()
//		Translate 608 current attributes into a 708 Pen Attributes struct
//
bool CNTV2CaptionTranslatorChannel608to708::GetCurrentPenAttributes (CC708PenAttr & outAttr) const
{
	//	Get the attributes for the current location...
	UWord	screen	(GetCurrentScreen ());
	if (GetCaptionMode () == NTV2_CC608_CapModePopOn)		// in PopUp mode, get attributes from offscreen array
		screen = (GetCurrentScreen () == 0 ? 1 : 0);

	NTV2Line21Attributes	attr;
	GetCharacterAttributes (screen, GetRow (), GetColumn (), attr);

	//	Most of the 708 attributes have no 608 counterparts, so start by setting all fields to the default values...
	Init608CCPenAttributes (outAttr, GetCaptionMode ());

	//	Change any that need changing...
	if (attr.IsSet ())
	{
		outAttr.italics	 = attr.IsItalicized ();
		outAttr.underline = attr.IsUnderlined ();
	}

	return true;

}	//	GetCurrentPenAttributes


// Convert608CharacterTo708()
//
//	This method is called when translating CEA-608 characters to CEA-708. It is used to compensate for any differences
//	in the two character sets.
bool CNTV2CaptionTranslatorChannel608to708::Convert608CharacterTo708 (const UByte inChar608, UByte & outChar708_1, UByte & outChar708_2)
{
	outChar708_1 = outChar708_2 = 0;	//	newChar2 is only used for 2-byte outputs (otherwise should return zero)

	//	Most single-byte characters are direct copies, with a handful of exceptions...
	switch (inChar608)
	{
		case 0x2a:	outChar708_1 = 0xe1;	break;		// lowercase a, acute accent
		case 0x5c:	outChar708_1 = 0xe9;	break;		// lowercase e, acute accent
		case 0x5e:	outChar708_1 = 0xed;	break;		// lowercase i, acute accent
		case 0x5f:	outChar708_1 = 0xf3;	break;		// lowercase o, acute accent
		case 0x60:	outChar708_1 = 0xfa;	break;		// lowercase u, acute accent
		case 0x7b:	outChar708_1 = 0xe7;	break;		// lowercase c, with cedilla
		case 0x7c:	outChar708_1 = 0xf7;	break;		// division symbol
		case 0x7d:	outChar708_1 = 0xd1;	break;		// uppercase N-tilde
		case 0x7e:	outChar708_1 = 0xf1;	break;		// lowercase n-tilde

		case 0x7f:	outChar708_1 = 0x10;
					outChar708_2 = 0x30;	break;		// solid block (2-byte)

		default:	outChar708_1 = inChar608;	break;		// copy original character
	}

	return true;

}	//	Convert608CharacterTo708


/*********************************************************************************************
 *
 *	CEA-708 Caption Command routines - generates command(s) and pushes them onto the queue
 *
 *	Note that each of the following routines generates a single self-contained "Service Block".
 *	Each Service Block can be considered an "atomic" command - it will be transmitted intact.
 *	There is no guarantee that separate Service Blocks will be sent in the same frame. This
 *	is not usually a concern in CEA-708 (the data is considered a "stream" and will all
 *	eventually get there in order...), but if you need a collection of data and/or commands
 *	to arrive together, you should bundle them all in the same Service Block and queue the
 *	single combined Service Block.
 *
 *	Note that these methods use a CNTV2CaptionEncoder708 class object as a "helper" to
 *	construct the 708 commands.
 *
 *********************************************************************************************/

// QueueServiceBlock_CharacterData()
//
//	Queue a Character Data Service Block in our output queue
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_CharacterData (int serviceNum, UByte ccChar)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (serviceNum));
	index += svcBlockHdrSize;

	//	Add the character...
	if (!m_708Encoder->MakeServiceBlockCharData (index, ccChar, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, serviceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;		//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_CharacterData



// QueueServiceBlock_TwoByteData()
//
//	Queue a Two Byte Service Block in our output queue
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_TwoByteData (int serviceNum, UByte ccChar1, UByte ccChar2)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (serviceNum));
	index += svcBlockHdrSize;

	//	Add the characters...
	if (!m_708Encoder->MakeServiceBlockCharData (index, ccChar1, index))
		return false;

	if (!m_708Encoder->MakeServiceBlockCharData (index, ccChar2, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, serviceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;	//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_TwoByteData



// QueueServiceBlock_SetCurrentWindow()
//
//	Queue a "Set Current Window" (CW0 - CW7) Service Block in our output queue (See CEA-708C pg 62)
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_SetCurrentWindow (int serviceNum, int windowID)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (serviceNum));
	index += svcBlockHdrSize;

	//	Add the SetCurrentWindow command...
	if (!m_708Encoder->MakeSetCurrentWindowCommand (index, windowID, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, serviceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;	//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_SetCurrentWindow



// QueueServiceBlock_DefineWindow()
//
//	Queue a "Define Window" (DF0 - DF7) Service Block in our output queue (See CEA-708C pg 63)
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_DefineWindow (const int inServiceNum, const int inWindowID, const CC708WindowParms & inParms)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (inServiceNum));
	index += svcBlockHdrSize;

	//	Add the DefineWindow command...
	if (!m_708Encoder->MakeDefineWindowCommand (index, inWindowID, inParms, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, inServiceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;	//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_DefineWindow



// QueueServiceBlock_ClearWindows()
//
//	Queue a "Clear Windows" (CLW) Service Block in our output queue (See CEA-708C pg 68)
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_ClearWindows (int serviceNum, UByte windowMap)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (serviceNum));
	index += svcBlockHdrSize;

	//	Add the ClearWindows command...
	if (!m_708Encoder->MakeClearWindowsCommand (index, windowMap, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, serviceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;	//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_ClearWindows



// QueueServiceBlock_DeleteWindows()
//
//	Queue a "Delete Windows" (DLW) Service Block in our output queue (See CEA-708C pg 69)
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_DeleteWindows (int serviceNum, UByte windowEnables)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (serviceNum));
	index += svcBlockHdrSize;

	//	Add the DeleteWindows command...
	if (!m_708Encoder->MakeDeleteWindowsCommand (index, windowEnables, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, serviceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;	//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_DeleteWindows



// QueueServiceBlock_DisplayWindows()
//
//	Queue a "Display Windows" (DSW) Service Block in our output queue (See CEA-708C pg 70)
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_DisplayWindows (int serviceNum, UByte windowMap)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (serviceNum));
	index += svcBlockHdrSize;

	//	Add the DisplayWindows command...
	if ( !m_708Encoder->MakeDisplayWindowsCommand (index, windowMap, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, serviceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;	//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_DisplayWindows



// QueueServiceBlock_HideWindows()
//
//	Queue a "Hide Windows" (HDW) Service Block in our output queue (See CEA-708C pg 71)
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_HideWindows (int serviceNum, UByte windowMap)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (serviceNum));
	index += svcBlockHdrSize;

	//	Add the HideWindows command...
	if (!m_708Encoder->MakeHideWindowsCommand(index, windowMap, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, serviceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;	//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_HideWindows



// QueueServiceBlock_ToggleWindows()
//
//	Queue a "Toggle Windows" (TGW) Service Block in our output queue (See CEA-708C pg 72)
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_ToggleWindows (int serviceNum, UByte windowMap)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (serviceNum));
	index += svcBlockHdrSize;

	//	Add the ToggleWindows command...
	if (!m_708Encoder->MakeToggleWindowsCommand(index, windowMap, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, serviceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;	//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_ToggleWindows



// QueueServiceBlock_SetWindowAttributes()
//
//	Queue a "Set Window Attributes" (SWA) Service Block in our output queue (See CEA-708C pg 73)
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_SetWindowAttributes (const int inServiceNum, const CC708WindowAttr & inAttr)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (inServiceNum));
	index += svcBlockHdrSize;

	//	Add the Set Window Attributes command...
	if (!m_708Encoder->MakeSetWindowAttributesCommand (index, inAttr, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, inServiceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);
	return true;	//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_SetWindowAttributes



// QueueServiceBlock_SetPenAttributes()
//
//	Queue a "Set Pen Attributes" (SPA) Service Block in our output queue (See CEA-708C pg 76)
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_SetPenAttributes (const int inServiceNum, const CC708PenAttr & inAttr)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (inServiceNum));
	index += svcBlockHdrSize;

	//	Add the Set Pen Attributes command...
	if (!m_708Encoder->MakeSetPenAttributesCommand (index, inAttr, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, inServiceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue (if channel enabled)...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;	//	Not an error if channel disabled

}	//	QueueServiceBlock_SetPenAttributes



// QueueServiceBlock_SetPenColor()
//
//	Queue a "Set Pen Color" (SPC) Service Block in our output queue (See CEA-708C pg 78)
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_SetPenColor (const int inServiceNum, const CC708PenColor & inColor)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (inServiceNum));
	index += svcBlockHdrSize;

	//	Add the Set Pen Color command...
	if (!m_708Encoder->MakeSetPenColorCommand (index, inColor, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, inServiceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;	//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_SetPenColor



// QueueServiceBlock_SetPenLocation()
//
//	Queue a "Set Pen Location" (SPL) Service Block in our output queue (See CEA-708C pg 79)
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_SetPenLocation (const int inServiceNum, const CC708PenLocation & inLoc)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (inServiceNum));
	index += svcBlockHdrSize;

	//	Add the Set Pen Location command...
	if (!m_708Encoder->MakeSetPenLocationCommand (index, inLoc, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, inServiceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;	//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_SetPenLocation



// QueueServiceBlock_Delay()
//
//	Queue a "Delay" (DLY) Service Block in our output queue (See CEA-708C pg 80)
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_Delay (int serviceNum, const UByte delay)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (serviceNum));
	index += svcBlockHdrSize;

	//	Add the Delay command...
	if (!m_708Encoder->MakeDelayCommand (index, delay, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, serviceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;	//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_Delay



// QueueServiceBlock_DelayCancel()
//
//	Queue a "Delay Cancel" (DLC) Service Block in our output queue (See CEA-708C pg 81)
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_DelayCancel (int serviceNum)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (serviceNum));
	index += svcBlockHdrSize;

	//	Add the Delay Cancel command...
	if (!m_708Encoder->MakeDelayCancelCommand (index, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, serviceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;	//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_DelayCancel


// QueueServiceBlock_Reset()
//
//	Queue a "Reset" (RST) Service Block in our output queue (See CEA-708C pg 82)
//
bool CNTV2CaptionTranslatorChannel608to708::QueueServiceBlock_Reset (int serviceNum)
{
	size_t	svcBlockSize	(0);
	size_t	index			(0);

	//	Clear buffer and init size...
	m_708Encoder->InitCaptionChannelPacket ();

	//	Skip the Service Block Header until we know how big the block is...
	const size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (serviceNum));
	index += svcBlockHdrSize;

	//	Add the Reset command...
	if (!m_708Encoder->MakeResetCommand (index, index))
		return false;

	//	Now that we know how big the Service Block is, insert the header at the beginning...
	svcBlockSize = index;
	if (!m_708Encoder->MakeServiceBlockHeader (0, serviceNum, svcBlockSize - svcBlockHdrSize))
		return false;

	//	Determine Service Block size...
	m_708Encoder->SetCaptionChannelPacketSize (svcBlockSize);

	//	Add it to the Service Block Queue...
	if (m_708TranslateEnable)
		return m_SvcBlockQueue.PushServiceBlock (m_708Encoder->GetCaptionChannelPacket (), svcBlockSize);

	return true;	//	It's not an error if this channel is disabled

}	//	QueueServiceBlock_Reset



// GetNextServiceBlockInfoFromQueue()
//
//	Returns the Block Size, Data Size, and Service Number of the next Service Block on the queue (returns false if empty).
//	This is a "peek" operation - the service block is left on the queue.
//
bool CNTV2CaptionTranslatorChannel608to708::GetNextServiceBlockInfoFromQueue (size_t & outBlockSize, size_t & outDataSize, int & outServiceNum, bool & outIsExtended)
{
	return m_SvcBlockQueue.PeekNextServiceBlockInfo (outBlockSize, outDataSize, outServiceNum, outIsExtended);
}



// GetNextServiceBlockFromQueue()
//
//	Copies the next Service Block from the queue to the designated pointer (also "pops" queue element).
//	This method copies the entire Service Block - to copy only the data, call GetNextServiceBlockDataFromQueue().
//	Returns the size of the copied data.
//
size_t CNTV2CaptionTranslatorChannel608to708::GetNextServiceBlockFromQueue (UByte * pOutDataBuffer)
{
	return m_SvcBlockQueue.PopServiceBlock (pOutDataBuffer);
}



// GetNextServiceBlockDataFromQueue()
//
//	Copies the next Service Block (data only) from the queue to the designated pointer (also "pops" queue element).
//	This method only copies the Service Block payload - to copy the entire Service Block, call GetNextServiceBlockFromQueue().
//	Returns the size of the copied data.
//
size_t CNTV2CaptionTranslatorChannel608to708::GetNextServiceBlockDataFromQueue (UByte * pOutDataBuffer)
{
	return m_SvcBlockQueue.PopServiceBlockData (pOutDataBuffer);
}
