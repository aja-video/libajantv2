/**
	@file		ntv2captiontranslatorchannel608to708.h
	@brief		Declares the CNTV2CaptionTranslatorChannel608to708 class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CEA608to708_TRANSLATORCHANNEL_
#define __NTV2_CEA608to708_TRANSLATORCHANNEL_

#include "ntv2captiondecodechannel608.h"
#include "ntv2captionencoder708.h"
#include "ntv2caption708serviceblockqueue.h"


#ifdef MSWindows
	#include "windows.h"
	#include "stdio.h"
#endif


//	Number of 708 screen coordinate "cells" per 608 text row (i.e. 5)
const int NTV2_CC608_TextRowHeight	= (NTV2_CC708ScreenCellHeight / NTV2_CC608_MaxRow);


const int NTV2_CC708DefaultPopOnWindowID   = 0;		//	The first of NTV2_CC708NumPopOnWindows consecutive indices alternately used for PopOn mode
const int NTV2_CC708NumPopOnWindows		   = 2;

const int NTV2_CC708DefaultRollUpWindowID  = 0;
const int NTV2_CC708DefaultPaintOnWindowID = 0;
const int NTV2_CC708DefaultTextWindowID	   = 2;


//	Default 608 Window Parms
const int NTV2_CC708Default608WindowPriority	  = 3;
const int NTV2_CC708DefaultRollUpAnchorH 		  =  0;
const int NTV2_CC708DefaultRollUpAnchorV 		  = 85;

const int NTV2_CC708Default608PopOnWindowStyleID  = 2;	//	Window Styles depend on mode
const int NTV2_CC708Default608RollUpWindowStyleID = 2;
const int NTV2_CC708Default608TextWindowStyleID   = 4;

const int NTV2_CC708Default608PopOnPenStyleID	  = 4;	//	Pen Styles depend on mode
const int NTV2_CC708Default608RollUpPenStyleID	  = 0;
const int NTV2_CC708Default608TextPenStyleID	  = 4;


//	Default 608 Window Attributes
const int NTV2_CC708Default608Justify		  = NTV2_CC708JustifyLeft;
const int NTV2_CC708Default608PrintDir		  = NTV2_CC708PrintDirLtoR;
const int NTV2_CC708Default608ScrollDir		  = NTV2_CC708ScrollDirBtoT;
const int NTV2_CC708Default608WordWrap		  = NTV2_CC708NoWordWrap;
const int NTV2_CC708Default608DisplayEffect	  = NTV2_CC708DispEffectSnap;
const int NTV2_CC708Default608EffectDir		  = NTV2_CC708EffectDirLtoR;
const int NTV2_CC708Default608EffectSpeed	  = 2;
const int NTV2_CC708Default608TextEffectSpeed = 1;
const int NTV2_CC708Default608BorderType	  = NTV2_CC708BorderTypeNone;


//	Default 608 Pen Attributes
const int NTV2_CC708Default608PenSize		= NTV2_CC708PenSizeStandard;
const int NTV2_CC708Default608FontStyle		= NTV2_CC708FontStyleMonoSansSerif;
const int NTV2_CC708Default608TextTag		= NTV2_CC708TextTagDialog;
const int NTV2_CC708Default608PenOffset		= NTV2_CC708PenOffsetNormal;
const int NTV2_CC708Default608Italics		= NTV2_CC708NoItalics;
const int NTV2_CC708Default608Underline		= NTV2_CC708NoUnderline;
const int NTV2_CC708Default608PenEdgeType	= NTV2_CC708PenEdgeTypeNone;


const int NTV2_CC708Default608Opacity		= NTV2_CC708OpacitySolid;



//	composite struct to hold status for each window
//
typedef struct CC708WindowStatus
{
	bool				bDefined;		///	True if this window has been "defined" with a DefineWindow command
	bool				bDirty;			///	True if window has any content (i.e. characters) written to it
	CC708WindowParms	windowParms;	///	
	CC708WindowAttr		windowAttr;
	CC708PenAttr		penAttr;
	CC708PenColor		penColor;
	CC708PenLocation	penLoc;

} CC708WindowStatus;	//	, *CC708WindowStatusPtr;



/**
	@brief	I translate a single channel of CEA-608 ("Line 21") closed captioning.
			Because CEA-608 allows for up to eight active "captioning channels" (CC1 - CC4, Text1 - Text4)
			at a given time, eight instances of me (one per CEA-608 caption channel) are used in a single
			CNTV2CaptionTranslator608to708 object, which parses the caption data stream and decides
			which channel to send the data to. It's important that I be called with data that is pertinent
			to the channel I've been asked to translate.

			I'm a subclass of CNTV2CaptionDecodeChannel608, which does most of the work of parsing and
			managing the state of the 608 captioning channel. I override most of the "state change"
			calls to additionally provide translations to CEA-708 data. As such, Parse608Data is my super-
			class' main high-level method, which takes two bytes of data (the most any one 608 channel
			can receive in a video frame) and decodes those bytes. As my superclass makes calls to modify
			its state, I override them and make the necessary translations to 708.

			In addition to the CEA-608 caption state maintained by my superclass, I maintain the state of
			the various "windows" used by the 708 caption decoder. The state of these windows typically
			form the parameters of the output 708 commands.

			After calling Parse608Data with each frame's 608 data, any translated commands are saved as
			CEA-708 "Service Blocks" in a FIFO queue. Clients can call GetNextServiceBlockInfoFromQueue
			to get the status of the queue and its Service Block (if any). Service Blocks can be dequeued
			by calling GetNextServiceBlockFromQueue.

	@note	I package each 708 command into its own self-contained Service Block. However, when putting
			together data for a 708 Caption Channel Packet, multiple commands targeted to the same
			Service Number can be put into a single Service Block. Because I have no visibility of this
			higher-level packet "packaging", it is up to the client to concatenate separate Service Blocks
			from me into a single combined Service Block when needed. To aid this effort, my
			GetNextServiceBlockDataFromQueue method may be called to dequeue a Service Block but only copy
			the Service Block's data payload.
**/
class CNTV2CaptionTranslatorChannel608to708;
typedef AJARefPtr <CNTV2CaptionTranslatorChannel608to708>	CNTV2CaptionTranslatorChannel608to708Ptr;

class CNTV2CaptionTranslatorChannel608to708 : public CNTV2CaptionDecodeChannel608
{
	//	CLASS METHODS
	public:
		static bool		Create (CNTV2CaptionTranslatorChannel608to708Ptr & outInstance);


	//	INSTANCE METHODS
	public:
		virtual					~CNTV2CaptionTranslatorChannel608to708 ();

		virtual void			Reset (void);

		virtual void			Set708ServiceNumber (int serviceNum);
		virtual inline int		Get708ServiceNumber (void) const				{return m_708ServiceNum;}

		virtual void			Set708TranslateEnable (bool enable);
		virtual inline bool		Get708TranslateEnable (void) const				{return m_708TranslateEnable;}

		virtual bool			SetChannel (const NTV2Line21Channel inChannel);		//	Override


		virtual bool	Init608CCWindowStatus	  (int winID,  NTV2Line21Mode mode = NTV2_CC608_CapModePopOn);
		virtual bool	Init608CCWindowParms	  (CC708WindowParms & outParms,	const NTV2Line21Mode inMode = NTV2_CC608_CapModePopOn) const;
		virtual bool	Init608CCWindowAttributes (CC708WindowAttr & outAttr,	const NTV2Line21Mode inMode = NTV2_CC608_CapModePopOn) const;
		virtual bool	Init608CCPenAttributes	  (CC708PenAttr & outAttr,		const NTV2Line21Mode inMode = NTV2_CC608_CapModePopOn) const;
		virtual bool	Init608CCPenColor		  (CC708PenColor & outColor,	const NTV2Line21Mode inMode = NTV2_CC608_CapModePopOn) const;
		virtual bool	Init608CCPenLocation	  (CC708PenLocation & outLoc,	const NTV2Line21Mode inMode = NTV2_CC608_CapModePopOn) const;

		virtual bool	DeleteWindow (int winID);
		virtual bool	IsWindowDefined (const int winID) const;
		virtual bool	IsWindowDirty (const int winID) const;

		virtual bool	GetCurrentEditWindowID (int * pWindowID = NULL, NTV2Line21Mode mode = NTV2_CC608_CapModeUnknown, bool * pbNewWindow = NULL);
		virtual int		GetCurrentModeWindowID (void);

		virtual bool	GetNextServiceBlockInfoFromQueue (size_t & outBlockSize, size_t & outDataSize, int & outServiceNum, bool & outIsExtended);
		virtual size_t	GetNextServiceBlockFromQueue (UByte * pOutDataBuffer);
		virtual size_t	GetNextServiceBlockDataFromQueue (UByte * pOutDataBuffer);


	//	PROTECTED METHODS
	protected:

		virtual bool 	Parse608CharacterData		(UByte char608_1, UByte char608_2, std::string & outDebugStr);
		virtual bool	Parse608TabOffsetCommand	(UByte char608_1, UByte char608_2, std::string & outDebugStr);
		virtual bool	Parse608CharacterSetCommand	(UByte char608_1, UByte char608_2, std::string & outDebugStr);
		virtual bool	Parse608AttributeCommand	(UByte char608_1, UByte char608_2, std::string & outDebugStr);
		virtual bool	Parse608PACCommand			(UByte char608_1, UByte char608_2, std::string & outDebugStr);
		virtual bool	Parse608MidRowCommand		(UByte char608_1, UByte char608_2, std::string & outDebugStr);
		virtual bool	Parse608SpecialCharacter	(UByte char608_1, UByte char608_2, std::string & outDebugStr);

		virtual bool	DoBackspace (void);
		virtual bool	DoDeleteToEndOfRow (void);
		virtual bool	DoRollUpCaption (const UWord inRowCount);
		virtual bool	DoFlashOn (void);
		virtual bool	DoTextRestart (void);
		virtual bool	DoResumeTextDisplay (void);
		virtual bool	DoEraseDisplayedMemory (void);
		virtual bool	DoCarriageReturn (void);
		virtual bool	DoEraseNonDisplayedMemory (void);
		virtual bool	DoEndOfCaption (void);

		virtual bool	GetCurrentPenLocation (CC708PenLocation & outLoc, int inWindowID = NTV2_CC708WindowIDMin - 1) const;
		virtual UWord	GetWindowRowOffset (int windowID) const;
		virtual bool	GetCurrentPenColor (CC708PenColor & outColor) const;
		virtual bool	GetCurrentPenAttributes (CC708PenAttr & outAttr) const;

		static bool		Convert608CharacterTo708 (const UByte inChar608, UByte & outChar708_1, UByte & outChar708_2);

		virtual bool	QueueServiceBlock_CharacterData (int serviceNum, UByte ccChar);
		virtual bool	QueueServiceBlock_TwoByteData (int serviceNum, UByte ccChar1, UByte ccChar2);
		virtual bool	QueueServiceBlock_SetCurrentWindow (int serviceNum, int windowID);
		virtual bool	QueueServiceBlock_DefineWindow (const int inServiceNum, const int inWindowID, const CC708WindowParms & inParms);
		virtual bool	QueueServiceBlock_ClearWindows (int serviceNum, UByte windowMap);
		virtual bool	QueueServiceBlock_DeleteWindows (int serviceNum, UByte windowEnables);
		virtual bool	QueueServiceBlock_DisplayWindows (int serviceNum, UByte windowMap);
		virtual bool	QueueServiceBlock_HideWindows (int serviceNum, UByte windowMap);
		virtual bool	QueueServiceBlock_ToggleWindows (int serviceNum, UByte windowMap);
		virtual bool	QueueServiceBlock_SetWindowAttributes (const int inServiceNum, const CC708WindowAttr & inAttr);
		virtual bool	QueueServiceBlock_SetPenAttributes (const int inServiceNum, const CC708PenAttr & inAttr);
		virtual bool	QueueServiceBlock_SetPenColor (const int inServiceNum, const CC708PenColor & inColor);
		virtual bool	QueueServiceBlock_SetPenLocation (const int inServiceNum, const CC708PenLocation & pLoc);
		virtual bool	QueueServiceBlock_Delay (int serviceNum, const UByte delay);
		virtual bool	QueueServiceBlock_DelayCancel (int serviceNum);
		virtual bool	QueueServiceBlock_Reset (int serviceNum);


	//	PRIVATE METHODS
	private:
		//	Hidden constructor, copy constructor, and assignment operator
		explicit		CNTV2CaptionTranslatorChannel608to708 ();
		explicit		CNTV2CaptionTranslatorChannel608to708 (const CNTV2CaptionTranslatorChannel608to708 & inObj);
		virtual CNTV2CaptionTranslatorChannel608to708 & operator = (const CNTV2CaptionTranslatorChannel608to708 & inObj);


	//	INSTANCE DATA
	private:
		int		m_708ServiceNum;					///< @brief	My 708 caption service number
		int		m_608PopOn_OnScreenWindowID;		///< @brief	Current on-air 608 "Pop-On" window ID (-1 if undefined)
		int		m_608PopOn_OffScreenWindowID;		///< @brief	Current off-air 608 "Pop-On" window ID (-1 if undefined)
		int		m_608RollUp_WindowID;				///< @brief	RollUp mode window ID (-1 if undefined)
		int		m_608PaintOn_WindowID;				///< @brief	PaintOn mode window ID (-1 if undefined)
		int		m_608Text_WindowID;					///< @brief	Text mode window ID (-1 if undefined)
		int		m_lastUsedWindowID;					///< @brief	Not strictly needed - used so we can alternate windows between 0 & 1 (more closely matches model)
		bool	m_708TranslateEnable;				///< @brief	True when this channel is part of the translated 708 output

		CC708WindowStatus					m_windowStatus [NTV2_CC708NumWindows];	///< @brief	Status of each possible 708 window
		CNTV2CaptionEncoder708Ptr			m_708Encoder;							///< @brief	CEA-708 encoder used to create 708 messages
		CNTV2Caption708ServiceBlockQueue	m_SvcBlockQueue;						///< @brief	Queue for outbound 708 Service Blocks

};	//	CNTV2CaptionTranslatorChannel608to708

#endif	// __NTV2_CEA608to708_TRANSLATORCHANNEL_
