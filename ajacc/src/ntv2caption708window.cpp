/**
	@file		ntv2caption708window.cpp
	@brief		Implementation of the CNTV2Caption708Window class.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc. All rights reserved.
**/
#include "ntv2caption708window.h"
#include <sstream>
#include <string.h>	//	for memset

#ifdef MSWindows
	#pragma warning(disable:4800)
	#pragma warning(disable:4127)	//	Stop MSVC from complaining about "do{...}while(false)" macros
#endif

//	Logging for this module...
#define LOGMYINFO(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Window, AJA_DebugSeverity_Info,	GetLogLabel() << GetChannelString() << ":  " << __xpr__)
#define	LOGMYERROR(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Window, AJA_DebugSeverity_Error,	GetLogLabel() << ":  " << AJAFUNC << ":  " << __xpr__)
#define	LOGMYDEBUG(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Window, AJA_DebugSeverity_Debug,	GetLogLabel() << ":  " << AJAFUNC << ":  " << __xpr__)

using namespace std;

static uint32_t gInstanceTally(0);


/////////////////////////////////////////////////////////////////////////////
// Constructor
//
CNTV2Caption708Window::CNTV2Caption708Window (void)
	:	m_windowID	(0),
		m_bDefined	(false),
		m_bDirty	(false)
{
	AJAAtomic::Increment(&gInstanceTally);
	ostringstream oss; oss << "Caption708Window-" << gInstanceTally;
	SetLogLabel(oss.str());

	m_windowParams.Clear();
	m_windowAttr.Clear();
	m_penAttr.Clear();
	m_penColor.Clear();
	m_penLoc.Reset();
	InitWindow();
}


CNTV2Caption708Window::~CNTV2Caption708Window ()
{
}



// InitWindow()
//	Sets all window parameters and text to its default state
//
void CNTV2Caption708Window::InitWindow (const int id)
{
	m_windowID = id;
	m_bDefined = false;
	m_bDirty   = false;

	ostringstream	oss;	oss << "[" << m_windowID << "]";
	AppendToLogLabel (oss.str ());

	m_windowParams.Clear();
	m_windowAttr.Clear();
	m_penAttr.Clear();
	m_penColor.Clear();
	m_penLoc.Reset();
	EraseWindowText();
}


// EraseWindowText()
//	Clear text from designated window
//
void CNTV2Caption708Window::EraseWindowText (void)
{
	// TBD...
}


// SetVisible()
//	Sets the visibility flag for this window
//
void CNTV2Caption708Window::SetVisible (const bool bVisible)
{
	m_windowParams.visible = bVisible;
}


// GetVisible()
//	Sets the visibility flag for this window
//
bool CNTV2Caption708Window::GetVisible (void)
{
	return m_windowParams.visible;
}



// DefineWindow()
//
void CNTV2Caption708Window::DefineWindow (const CC708WindowParms & inParms)
{
	m_windowParams = inParms;
	m_bDefined = true;
}

// SetWindowAttributes()
//
void CNTV2Caption708Window::SetWindowAttributes (const CC708WindowAttr & inAttr)
{
	if (m_bDefined)
		m_windowAttr = inAttr;
}

// SetPenAttributes()
//
void CNTV2Caption708Window::SetPenAttributes (const CC708PenAttr & inAttr)
{
	if (m_bDefined)
		m_penAttr = inAttr;
}

// SetPenLocation()
//
void CNTV2Caption708Window::SetPenLocation (const CC708PenLocation & inLoc)
{
	if (m_bDefined)
		m_penLoc = inLoc;
}


// SetPenColor()
//
void CNTV2Caption708Window::SetPenColor (const CC708PenColor & inColor)
{
	if (m_bDefined)
		m_penColor = inColor;
}


// AddCharacter()
//
void CNTV2Caption708Window::AddCharacter (const UByte inChar, const CC708CodeGroup inCodeGroup)
{
	(void)inChar;
	(void)inCodeGroup;
	// TBD...
}


// DoETX()
//
void CNTV2Caption708Window::DoETX (void)
{
	// TBD...
}


// DoBS()
//
void CNTV2Caption708Window::DoBS (void)
{
	// TBD...
}


// DoFF()
//
void CNTV2Caption708Window::DoFF (void)
{
	// TBD...
}


// DoCR()
//
void CNTV2Caption708Window::DoCR (void)
{
	// TBD...
}


// DoHCR()
//
void CNTV2Caption708Window::DoHCR (void)
{
	// TBD...
}


string NTV2_CC708OpacityToString (const NTV2_CC708Opacity inOpacity, const bool inCompact)
{
	switch (inOpacity)
	{
		case NTV2_CC708OpacitySolid:		return inCompact ? "O" : "opaq";
		case NTV2_CC708OpacityFlash:		return inCompact ? "F" : "flsh";
		case NTV2_CC708OpacityTranslucent:	return inCompact ? "L" : "tnsl";
		case NTV2_CC708OpacityTransparent:	return inCompact ? "P" : "tnsp";
		default:							break;
	}
	return inCompact ? "?" : "????";
}


CC708Color::CC708Color (const int inRed, const int inGreen, const int inBlue, const int inOpacity)
	:	red		(inRed),
		green	(inGreen),
		blue	(inBlue),
		opacity	(inOpacity)
{
	AJACC_ASSERT (IsValid ());
}


ostream & operator << (ostream & inOutStream, const CC708Color & inData)
{
	inOutStream	<< "R" << inData.red << "G" << inData.green << "B" << inData.blue
				<< "O" << ::NTV2_CC708OpacityToString (NTV2_CC708Opacity (inData.opacity));
	return inOutStream;
}



ostream & operator << (ostream & inOutStream, const CC708WindowParms & inData)
{
	return inOutStream
						<< "     priority=" << inData.priority				<< endl
						<< "     anchorPt=" << inData.anchorPt				<< endl
						<< "  relativePos=" << inData.relativePos			<< endl
						<< "      anchorV=" << inData.anchorV				<< endl
						<< "      anchorH=" << inData.anchorH				<< endl
						<< "     rowCount=" << inData.rowCount << "(+1)"	<< endl
						<< "     colCount=" << inData.colCount << "(+1)"	<< endl
						<< "      rowLock=" << inData.rowLock				<< endl
						<< "      colLock=" << inData.colLock				<< endl
						<< "      visible=" << inData.visible				<< endl
						<< "windowStyleID=" << inData.windowStyleID			<< endl
						<< "   penStyleID=" << inData.penStyleID			<< endl;
}


CC708WindowParms::CC708WindowParms ()
{
	Clear();
	AJACC_ASSERT (IsValid ());
}

void CC708WindowParms::Clear (void)
{
	priority		= NTV2_CC708DefaultWindowPriority;
	anchorPt		= NTV2_CC708DefaultWindowAnchorPt;
	relativePos		= NTV2_CC708DefaultRelativePos;
	anchorV			= NTV2_CC708DefaultAnchorV;
	anchorH			= NTV2_CC708DefaultAnchorH;
	rowCount		= NTV2_CC708DefaultRowCount;
	colCount		= NTV2_CC708DefaultColCount;
	rowLock			= NTV2_CC708DefaultRowLock;
	colLock			= NTV2_CC708DefaultColLock;
	visible			= NTV2_CC708DefaultVisible;
	windowStyleID	= NTV2_CC708DefaultWindowStyleID;
	penStyleID		= NTV2_CC708DefaultPenStyleID;
}

CC708WindowParms::CC708WindowParms (const UByte inParam1, const UByte inParam2, const UByte inParam3, const UByte inParam4, const UByte inParam5, const UByte inParam6)
	:	priority		(inParam1 & 0x07),
		anchorPt		((inParam4 & 0xF0) >> 4),
		relativePos		((inParam2 & 0x80) >> 7),
		anchorV			(inParam2 & 0x7F),
		anchorH			(inParam3 & 0xFF),
		rowCount		(inParam4 & 0x0F),
		colCount		(inParam5 & 0x3F),
		rowLock			((inParam1 & 0x10) >> 4),
		colLock			((inParam1 & 0x08) >> 3),
		visible			((inParam1 & 0x20) >> 5),
		windowStyleID	((inParam6 & 0x38) >> 3),
		penStyleID		(inParam6 & 0x07)
{
	AJACC_ASSERT (IsValid ());
}


bool CC708WindowParms::IsValid (void) const
{
	return	priority >= NTV2_CC708WindowPriorityMin		&& priority <= NTV2_CC708WindowPriorityMax
		&&	anchorPt >= NTV2_CC708WindowAnchorPointMin	&& anchorPt <= NTV2_CC708WindowAnchorPointMax
		&&	anchorV  >= 0								&& anchorV  <= 127
		&&	anchorH  >= 0								&& anchorH  <= 255
		&&	rowCount >= 1								&& rowCount <= 16
		&&	colCount >= 1								&& colCount <= 64
		&&	windowStyleID >= NTV2_CC708WindowStyleIDMin	&& windowStyleID <= NTV2_CC708WindowStyleIDMax
		&&	penStyleID    >= NTV2_CC708PenStyleIDMin	&& penStyleID    <= NTV2_CC708PenStyleIDMax;
}


CC708WindowAttr::CC708WindowAttr ()
{
	Clear();
	AJACC_ASSERT (IsValid ());
}

void CC708WindowAttr::Clear (void)
{
	justify			= NTV2_CC708DefaultJustify;
	printDir		= NTV2_CC708DefaultPrintDir;
	scrollDir		= NTV2_CC708DefaultScrollDir;
	wordWrap		= NTV2_CC708DefaultWordWrap;
	displayEffect	= NTV2_CC708DefaultDisplayEffect;
	effectDir		= NTV2_CC708DefaultEffectDir;
	effectSpeed		= NTV2_CC708DefaultEffectSpeed;
	borderType		= NTV2_CC708DefaultBorderType;
	fillColor		= NTV2_CC708DefaultFillColor;
	borderColor		= NTV2_CC708DefaultBorderColor;
}

CC708WindowAttr::CC708WindowAttr (const UByte inParam1, const UByte inParam2, const UByte inParam3, const UByte inParam4)
	:	justify			(inParam3 & 0x03),
		printDir		((inParam3 & 0x30) >> 4),
		scrollDir		((inParam3 & 0x0C) >> 2),
		wordWrap		((inParam3 & 0x40) >> 6),
		displayEffect	(inParam4 & 0x03),
		effectDir		((inParam4 & 0x0C) >> 2),
		effectSpeed		((inParam4 & 0xF0) >> 4),
		borderType		(((inParam3 & 0x80) >> 5) + ((inParam2 & 0xC0) >> 6)),
//						Red							Green						Blue				Opacity
		fillColor		((inParam1 & 0x30) >> 4,	(inParam1 & 0x0C) >> 2,		inParam1 & 0x03,	(inParam1 & 0xC0) >> 6),
		borderColor		((inParam2 & 0x30) >> 4,	(inParam2 & 0x0C) >> 2,		inParam2 & 0x03)
{
	AJACC_ASSERT (IsValid ());
}


bool CC708WindowAttr::IsValid (void) const
{
	return	   justify		 >= NTV2_CC708JustifyMin		&& justify		 <= NTV2_CC708JustifyMax
			&& printDir		 >= NTV2_CC708PrintDirMin		&& printDir		 <= NTV2_CC708PrintDirMax
			&& scrollDir	 >= NTV2_CC708ScrollDirMin		&& scrollDir	 <= NTV2_CC708ScrollDirMax
			&& displayEffect >= NTV2_CC708DispEffectMin		&& displayEffect <= NTV2_CC708DispEffectMax
			&& effectDir	 >= NTV2_CC708EffectDirMin		&& effectDir	 <= NTV2_CC708EffectDirMax
			&& effectSpeed	 >= 0							&& effectSpeed	 <= 15
			&& borderType	 >= NTV2_CC708BorderTypeMin		&& borderType	 <= NTV2_CC708BorderTypeMax
			&& fillColor.IsValid () && borderColor.IsValid ();
}


ostream & operator << (ostream & inOutStream, const CC708WindowAttr & inData)
{
	return inOutStream	<< "    fillColor=" <<	inData.fillColor		<< endl
						<< "   borderType=" <<	inData.borderType		<< endl
						<< "  borderColor=" <<	inData.borderColor		<< endl
						<< "     wordWrap=" <<	inData.wordWrap			<< endl
						<< "     printDir=" <<	inData.printDir			<< endl
						<< "    scrollDir=" <<	inData.scrollDir		<< endl
						<< "      justify=" <<	inData.justify			<< endl
						<< "  effectSpeed=" <<	inData.effectSpeed		<< endl
						<< "    effectDir=" <<	inData.effectDir		<< endl
						<< "displayEffect=" <<	inData.displayEffect	<< endl;
}


CC708PenAttr::CC708PenAttr ()
{
	Clear();
	AJACC_ASSERT (IsValid ());
}

void CC708PenAttr::Clear (void)
{
	penSize		= NTV2_CC708DefaultPenSize;
	fontStyle	= NTV2_CC708DefaultFontStyle;
	textTag		= NTV2_CC708DefaultTextTag;
	offset		= NTV2_CC708DefaultPenOffset;
	italics		= NTV2_CC708DefaultItalics;
	underline	= NTV2_CC708DefaultUnderline;
	edgeType	= NTV2_CC708DefaultPenEdgeType;
}


CC708PenAttr::CC708PenAttr (const UByte inParam1, const UByte inParam2)
	:	penSize		(inParam1 & 0x03),
		fontStyle	(inParam2 & 0x07),
		textTag		((inParam1 & 0xF0) >> 4),
		offset		((inParam1 & 0x0C) >> 2),
		italics		((inParam2 & 0x80) >> 7),
		underline	((inParam2 & 0x40) >> 6),
		edgeType	((inParam2 & 0x38) >> 3)
{
	AJACC_ASSERT (IsValid ());
}


ostream & operator << (ostream & inOutStream, const CC708PenAttr & inData)
{
	return inOutStream	<< "  textTag=" << inData.textTag	<< endl
						<< "   offset=" << inData.offset	<< endl
						<< "  penSize=" << inData.penSize	<< endl
						<< "  italics=" << inData.italics	<< endl
						<< "underline=" << inData.underline	<< endl
						<< " edgeType=" << inData.edgeType	<< endl
						<< "fontStyle=" << inData.fontStyle	<< endl;
}


CC708PenLocation::CC708PenLocation (const int inRow, const int inColumn)
	:	row		(inRow),
		column	(inColumn)
{
}


ostream & operator << (ostream & inOutStream, const CC708PenLocation & inData)
{
	return inOutStream << "R" << inData.row << "C" << inData.column;
}


CC708PenColor::CC708PenColor ()
{
	Clear();
	AJACC_ASSERT (IsValid ());
}

void CC708PenColor::Clear (void)
{
	fg		= NTV2_CC708WhiteColor;
	bg		= NTV2_CC708BlackColor;
	edge	= NTV2_CC708WhiteColor;
}

CC708PenColor::CC708PenColor (const UByte inParam1, const UByte inParam2, const UByte inParam3)
				//	Red						Green						Blue				Opacity
	:	fg		((inParam1 & 0x30) >> 4,	(inParam1 & 0x0C) >> 2,		inParam1 & 0x03,	(inParam1 & 0xC0) >> 6),
		bg		((inParam2 & 0x30) >> 4,	(inParam2 & 0x0C) >> 2,		inParam2 & 0x03,	(inParam2 & 0xC0) >> 6),
		edge	((inParam3 & 0x30) >> 4,	(inParam3 & 0x0C) >> 2,		inParam3 & 0x03)
{
	AJACC_ASSERT (IsValid ());
}


ostream & operator << (ostream & inOutStream, const CC708PenColor & inData)
{
	return inOutStream	<< "  fg=" << inData.fg << endl
						<< "  bg=" << inData.bg << endl
						<< "edge=" << inData.edge << endl;
}
