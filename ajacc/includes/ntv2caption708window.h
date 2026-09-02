/**
	@file		ntv2caption708window.h
	@brief		Declares the CNTV2Caption708Window class.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CEA708_WINDOW_
#define __NTV2_CEA708_WINDOW_

#include "ntv2captionencoder708.h"

#ifdef MSWindows
	#include "windows.h"
	#include "stdio.h"
#endif


// default 708 Window Parms
const int NTV2_CC708DefaultWindowPriority = 3;
const int NTV2_CC708DefaultWindowAnchorPt = NTV2_CC708WindowAnchorPointUpperLeft;
const int NTV2_CC708DefaultRelativePos	  = NTV2_CC708AbsolutePos;
const int NTV2_CC708DefaultAnchorH 		  = 0;
const int NTV2_CC708DefaultAnchorV 		  = 0;
const int NTV2_CC708DefaultRowCount		  = 1;
const int NTV2_CC708DefaultColCount		  = NTV2_CC608_MaxCol;
const int NTV2_CC708DefaultRowLock		  = NTV2_CC708Lock;
const int NTV2_CC708DefaultColLock		  = NTV2_CC708Lock;
const int NTV2_CC708DefaultVisible		  = NTV2_CC708NotVisible;
const int NTV2_CC708DefaultWindowStyleID  = 2;
const int NTV2_CC708DefaultPenStyleID	  = 4;


// default 608 Window Attributes
const int NTV2_CC708DefaultJustify			= NTV2_CC708JustifyLeft;	
const int NTV2_CC708DefaultPrintDir			= NTV2_CC708PrintDirLtoR;	
const int NTV2_CC708DefaultScrollDir		= NTV2_CC708ScrollDirBtoT;	
const int NTV2_CC708DefaultWordWrap			= NTV2_CC708NoWordWrap;	
const int NTV2_CC708DefaultDisplayEffect	= NTV2_CC708DispEffectSnap;	
const int NTV2_CC708DefaultEffectDir		= NTV2_CC708EffectDirLtoR;	
const int NTV2_CC708DefaultEffectSpeed		= 2;	
const int NTV2_CC708DefaultTextEffectSpeed	= 1;	
const int NTV2_CC708DefaultBorderType		= NTV2_CC708BorderTypeNone;	

const CC708Color NTV2_CC708DefaultFillColor	  = NTV2_CC708BlackColor;	
const CC708Color NTV2_CC708DefaultBorderColor = NTV2_CC708BlackColor;	


// default 608 Pen Attributes
const int NTV2_CC708DefaultPenSize		= NTV2_CC708PenSizeStandard;	
const int NTV2_CC708DefaultFontStyle	= NTV2_CC708FontStyleMonoSansSerif;	
const int NTV2_CC708DefaultTextTag		= NTV2_CC708TextTagDialog;	
const int NTV2_CC708DefaultPenOffset	= NTV2_CC708PenOffsetNormal;	
const int NTV2_CC708DefaultItalics		= NTV2_CC708NoItalics;	
const int NTV2_CC708DefaultUnderline	= NTV2_CC708NoUnderline;	
const int NTV2_CC708DefaultPenEdgeType	= NTV2_CC708PenEdgeTypeNone;


typedef enum CC708CodeGroup
{
	NTV2_CC708CodeGroup_C0,
	NTV2_CC708CodeGroup_G0,
	NTV2_CC708CodeGroup_C1,
	NTV2_CC708CodeGroup_G1,

	NTV2_CC708CodeGroup_C2,
	NTV2_CC708CodeGroup_G2,
	NTV2_CC708CodeGroup_C3,
	NTV2_CC708CodeGroup_G3

} CC708CodeGroup;


class AJAExport CNTV2Caption708Window : public CNTV2CaptionLogConfig
{
	//	INSTANCE METHODS
	public:
						CNTV2Caption708Window ();
		virtual			~CNTV2Caption708Window ();

		void			InitWindow (const int id = 0);

		inline void		SetWindowID (const int id)			{m_windowID = id;}

		void			SetVisible (const bool bVisible);
		bool			GetVisible (void);

		void			DefineWindow (const CC708WindowParms & inParms);
		void			SetWindowAttributes (const CC708WindowAttr & inAttr);
		void			SetPenAttributes (const CC708PenAttr & inAttr);
		void			SetPenLocation (const CC708PenLocation & inLoc);
		void			SetPenColor (const CC708PenColor & inColor);

		void			EraseWindowText (void);				///< @todo	Needs implementation
		void			AddCharacter (const UByte inChar, const CC708CodeGroup inCodeGroup);	///< @todo	Needs implementation
		void			DoETX (void);		///< @todo	Needs implementation
		void			DoBS (void);		///< @todo	Needs implementation
		void			DoFF (void);		///< @todo	Needs implementation
		void			DoCR (void);		///< @todo	Needs implementation
		void			DoHCR (void);		///< @todo	Needs implementation

	//	INSTANCE DATA
	private:
		int					m_windowID;			///< @brief	Which window I am
		bool				m_bDefined;			///< @brief	True if I've been "defined" with a DefineWindow command
		bool				m_bDirty;			///< @brief	True if I have any content (i.e. characters written into me)
		CC708WindowParms	m_windowParams;		///< @brief	My parameters
		CC708WindowAttr		m_windowAttr;		///< @brief	My attributes
		CC708PenAttr		m_penAttr;			///< @brief	My pen attributes
		CC708PenColor		m_penColor;			///< @brief	My pen color
		CC708PenLocation	m_penLoc;			///< @brief	My pen location

};	//	CNTV2Caption708Window

#endif	// __NTV2_CEA708_SERVICE_
