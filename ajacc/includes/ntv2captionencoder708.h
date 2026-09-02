/**
	@file		ntv2captionencoder708.h
	@brief		Declares the CNTV2CaptionEncoder708 class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CEA708_ENCODER_
#define __NTV2_CEA708_ENCODER_

#include "ntv2caption608types.h"
#include "ntv2caption708serviceinfo.h"
#include "ajabase/common/ajarefptr.h"

#ifdef MSWindows
	#include "windows.h"
	#include "stdio.h"
#endif

typedef UByte *		UBytePtr;
typedef UWord *		UWordPtr;


class CNTV2Caption708ServiceInfo;


// returns size of Service Block Header (1 byte for standard services, 2 bytes for extended)
#define ServiceBlockHeaderSize(svcNum) (svcNum <= 6 ? 1 : 2)


// Caption Channel Packets cannot be longer than 256 bytes (and still fit into a SMPTE-334 ANC packet payload)
const size_t NTV2_CC708MaxPktSize = 256;
const size_t NTV2_CC708MaxAncSize = 512;


// CEA-708 constants
const size_t NTV2_CC708_MaxCaptionChannelPacketSize	= 128;		// including Caption Channel Packet Header byte!
const size_t NTV2_CC708_MaxServiceBlockSize			=  31;		// NOT including Service Block Header byte(s)!


// CEA-708B Caption Data Packet section IDs
enum
{
	NTV2_CC708_CDPHeaderId1		= 0x96,		// cdp_identifier (2 bytes)
	NTV2_CC708_CDPHeaderId2		= 0x69,
	NTV2_CC708_CDPTimecodeId	= 0x71,		// time_code_section_id
	NTV2_CC708_CDPDataId		= 0x72,		// ccdata_id
	NTV2_CC708_CDPServiceInfoId	= 0x73,		// ccsvcinfo_id
	NTV2_CC708_CDPFooterId		= 0x74		// cdp_footer
};


// CEA-708B cdp_frame_rate enums (see CEA-708B, pg 72)
enum
{
	NTV2_CC708CDPFrameRate23p98	= 1,
	NTV2_CC708CDPFrameRate24	= 2,
	NTV2_CC708CDPFrameRate25	= 3,
	NTV2_CC708CDPFrameRate29p97	= 4,
	NTV2_CC708CDPFrameRate30	= 5,
	NTV2_CC708CDPFrameRate50	= 6,
	NTV2_CC708CDPFrameRate59p94	= 7,
	NTV2_CC708CDPFrameRate60	= 8
};


//	CEA-708B CDP Header flags (see CEA-708B, pp 72-73)
enum
{
	NTV2_CC708CDPHeader_TimeCodePresent		 = (1 << 7),	// set if CDP contains time_code_section
	NTV2_CC708CDPHeader_CCDataPresent		 = (1 << 6),	// set if CDP contains cc_data section
	NTV2_CC708CDPHeader_SvcInfoPresent		 = (1 << 5),	// set if CDP contains ccsvcinfo_section
	NTV2_CC708CDPHeader_SvcInfoStart		 = (1 << 4),
	NTV2_CC708CDPHeader_SvcInfoChange		 = (1 << 3),
	NTV2_CC708CDPHeader_SvcInfoComplete		 = (1 << 2),
	NTV2_CC708CDPHeader_CaptionServiceActive = (1 << 1),	// set if CDP contains an active caption service
	NTV2_CC708CDPHeader_Reserved			 = (1 << 0)		// this should always be "on"

};


// CEA-708 CDP "cc_type" enums
enum
{
	NTV2_CC708CCTypeNTSCField1	= 0,		// 608 Field 1
	NTV2_CC708CCTypeNTSCField2	= 1,		// 608 Field 2
	NTV2_CC708CCTypeDTVCCData	= 2,		// 708 (2nd, 3rd, 4th, etc.)
	NTV2_CC708CCTypeDTVCCStart	= 3			// 708 (1st)
};


typedef struct NTV2_CC708CDPHeader
{
	UWord	cdp_identifier;					// 0x9669
	UByte	cdp_length;						// length (in words) of entire packet
	int		cdp_frame_rate;					// see NTV2_CC708CDPFrameRateXXX enums
	int		cdp_flags;						// see NTV2_CC708CDPHeader_xxx enums
	UWord	cdp_hdr_sequence_cntr;			// sequence count (must match footer sequence count)

	inline void Zero (void)	{cdp_identifier = cdp_hdr_sequence_cntr = 0; cdp_length = 0; cdp_frame_rate = cdp_flags = 0;}
} NTV2_CC708CDPHeader, * NTV2_CC708CDPHeaderPtr;


typedef struct NTV2_CC708CDPTimecodeSection
{
	UByte	time_code_section_id;		// 0x71
	int		tc_10hrs;
	int		tc_1hrs;
	int		tc_10min;
	int		tc_1min;
	int		tc_10sec;
	int		tc_1sec;
	int		tc_10fr;
	int		tc_1fr;
	bool	tc_field_flag;
	bool	drop_frame_flag;

	inline void Zero (void)	{time_code_section_id = 0;
							tc_10hrs = tc_1hrs = tc_10min = tc_1min = tc_10sec = tc_1sec = tc_10fr = tc_1fr = 0;
							tc_field_flag = drop_frame_flag = false;}
} NTV2_CC708CDPTimecodeSection, * NTV2_CC708CDPTimecodeSectionPtr;


typedef struct NTV2_CC708CDPDataTriplet
{
	bool	cc_valid;
	int		cc_type;
	UByte	cc_data_1;
	UByte	cc_data_2;

	inline void Zero (void)	{cc_valid = false; cc_type = 0; cc_data_1 = cc_data_2 = 0;}
} NTV2_CC708CDPDataTriplet, NTV2_CC708CDPDataTripletPtr;


typedef struct NTV2_CC708CDPDataSection
{
	UByte						ccdata_id;		// 0x72
	int							cc_count;
	NTV2_CC708CDPDataTriplet	cc_data [32];

	void Zero (void);
} NTV2_CC708CDPDataSection, * NTV2_CC708CDPDataSectionPtr;


typedef struct NTV2_CC708CDPServiceInfoSection
{
	UByte					ccsvcinfo_id;   		// 0x73
	bool					svc_info_start;
	bool					svc_info_change;
	bool					svc_info_complete;
	int						svc_count;
	NTV2_CC708ServiceInfo	svc_info [16];

	void Zero (void);
} NTV2_CC708CDPServiceInfoSection, * NTV2_CC708CDPServiceInfoSectionPtr;


typedef struct NTV2_CC708CDPFooter
{
	UWord	cdp_ftr_sequence_cntr;			// sequence count (must match header sequence count)
	UByte	packet_checksum;

	inline void Zero (void)		{cdp_ftr_sequence_cntr = 0; packet_checksum = 0;}
} NTV2_CC708CDPFooter, * NTV2_CC708CDPFooterPtr;



typedef struct NTV2_CC708CDP
{
	NTV2_CC708CDPHeader				cdp_header;
	NTV2_CC708CDPTimecodeSection	timecode_section;
	NTV2_CC708CDPDataSection		ccdata_section;
	NTV2_CC708CDPServiceInfoSection	ccsvcinfo_section;
	NTV2_CC708CDPFooter				cdp_footer;

	inline void Zero (void)		{cdp_header.Zero(); timecode_section.Zero(); ccdata_section.Zero(); ccsvcinfo_section.Zero(); cdp_footer.Zero();}
} NTV2_CC708CDP, * NTV2_CC708CDPPtr;



// CEA-708B Screen Coordinates
const int NTV2_CC708ScreenCellWidth4x3	= 160;  	// max number of "cells" wide in a 4x3 screen format
const int NTV2_CC708ScreenCellWidth16x9	= 210;  	// max number of "cells" wide in a 16x9 screen format
const int NTV2_CC708ScreenCellHeight	= 75;  		// max number of "cells" in either screen format


enum
{
	NTV2_CC708WindowIDMin = 0,		// note: this MUST be zero or a whole lot of loops will break!
	NTV2_CC708WindowIDMax = 7,
	NTV2_CC708NumWindows
};



//	enums/constants for CC708Color struct
enum
{
	NTV2_CC708ColorMin = 0,
	NTV2_CC708ColorMax = 3
};


typedef enum
{
	NTV2_CC708OpacityMin		 = 0,
	NTV2_CC708OpacitySolid		 = NTV2_CC708OpacityMin,
	NTV2_CC708OpacityOpaque		 = NTV2_CC708OpacitySolid,
	NTV2_CC708OpacityFlash		 = 1,
	NTV2_CC708OpacityTranslucent = 2,
	NTV2_CC708OpacityTransparent = 3,
	NTV2_CC708OpacityMax		 = NTV2_CC708OpacityTransparent
} NTV2_CC708Opacity;


const int	NTV2_CC708DefaultOpacity	(NTV2_CC708OpacitySolid);


AJAExport std::string NTV2_CC708OpacityToString (const NTV2_CC708Opacity inOpacity, const bool inCompact = false);


typedef struct AJAExport CC708Color
{
	int		red;		// NTV2_CC708Color___ (0 - 3)
	int		green;		//
	int		blue;		//
	int		opacity;	// NTV2_CC708Opacity___

	explicit	CC708Color (const int inRed		= NTV2_CC708ColorMax,
							const int inGreen	= NTV2_CC708ColorMax,
							const int inBlue	= NTV2_CC708ColorMax,
							const int inOpacity	= NTV2_CC708DefaultOpacity);

	inline bool	IsValid (void) const		{return red >= NTV2_CC708ColorMin && red <= NTV2_CC708ColorMax
												&&  green >= NTV2_CC708ColorMin && green <= NTV2_CC708ColorMax
												&&  blue >= NTV2_CC708ColorMin && blue <= NTV2_CC708ColorMax
												&&  opacity >= NTV2_CC708OpacityMin && opacity <= NTV2_CC708OpacityMax;}
} CC708Color;

AJAExport std::ostream & operator << (std::ostream & inOutStream, const CC708Color & inData);

//	Commonly used opaque colors:			red						green				blue
const CC708Color	NTV2_CC708WhiteColor;
const CC708Color	NTV2_CC708GreenColor	(NTV2_CC708ColorMin,	NTV2_CC708ColorMax,	NTV2_CC708ColorMin);
const CC708Color	NTV2_CC708BlueColor		(NTV2_CC708ColorMin,	NTV2_CC708ColorMin,	NTV2_CC708ColorMax);
const CC708Color	NTV2_CC708CyanColor		(NTV2_CC708ColorMin,	NTV2_CC708ColorMax,	NTV2_CC708ColorMax);
const CC708Color	NTV2_CC708RedColor		(NTV2_CC708ColorMax,	NTV2_CC708ColorMin,	NTV2_CC708ColorMin);
const CC708Color	NTV2_CC708YellowColor	(NTV2_CC708ColorMax,	NTV2_CC708ColorMax,	NTV2_CC708ColorMin);
const CC708Color	NTV2_CC708MagentaColor	(NTV2_CC708ColorMax,	NTV2_CC708ColorMin,	NTV2_CC708ColorMax);
const CC708Color	NTV2_CC708BlackColor	(NTV2_CC708ColorMin,	NTV2_CC708ColorMin,	NTV2_CC708ColorMin);



// 708 Window Parameters - see CEA-708B, pg 41-42
//
enum
{
	NTV2_CC708WindowPriorityMin	= 0,
	NTV2_CC708WindowPriorityMax	= 7
};

const bool NTV2_CC708Visible	= true;
const bool NTV2_CC708NotVisible	= false;


enum
{
	NTV2_CC708WindowAnchorPointMin			= 0,
	NTV2_CC708WindowAnchorPointUpperLeft	= NTV2_CC708WindowAnchorPointMin,
	NTV2_CC708WindowAnchorPointUpperMiddle	= 1,
	NTV2_CC708WindowAnchorPointUpperRight	= 2,
	NTV2_CC708WindowAnchorPointCenterLeft	= 3,
	NTV2_CC708WindowAnchorPointCenterMiddle	= 4,
	NTV2_CC708WindowAnchorPointCenterRight	= 5,
	NTV2_CC708WindowAnchorPointLowerLeft	= 6,
	NTV2_CC708WindowAnchorPointLowerMiddle	= 7,
	NTV2_CC708WindowAnchorPointLowerRight	= 8,
	NTV2_CC708WindowAnchorPointMax			= NTV2_CC708WindowAnchorPointLowerRight
};


const bool NTV2_CC708AbsolutePos = false;
const bool NTV2_CC708RelativePos = true;

const bool NTV2_CC708Lock 	= true;
const bool NTV2_CC708NoLock = false;


enum
{
	NTV2_CC708WindowStyleIDMin = 0,
	NTV2_CC708WindowStyleIDMax = 7
};


enum
{
	NTV2_CC708PenStyleIDMin = 0,
	NTV2_CC708PenStyleIDMax = 7
};


typedef struct CC708WindowParms
{
	int		priority;			///	NTV2_CC708WindowPriority enums
	int		anchorPt;			///	NTV2_CC708WindowAnchorPoint enums
	bool	relativePos;		///	NTV2_CC708AbsolutePos/NTV2_CC708RelativePos
	int		anchorV;			///	0 - 127 (absolute position) or 0 - 99 (relative position)
	int		anchorH;			///	0 - 255 (absolute position) or 0 - 99 (relative position)
	int		rowCount;			///	number of rows - 1 (e.g. '0' = 1 row)
	int		colCount;			///	number of columns - 1 (e.g. '0' = 1 column)
	bool	rowLock;			///	NTV2_CC708NoLock/NTV2_CC708Lock
	bool	colLock;			///	NTV2_CC708NoLock/NTV2_CC708Lock
	bool	visible;			///	NTV2_CC708Visible/NTV2_CC708NotVisible
	int		windowStyleID;		///	NTV2_CC708WindowStyleID enums
	int		penStyleID;			///	NTV2_CC708PenStyleID enums

	explicit	CC708WindowParms ();
	explicit	CC708WindowParms (const UByte inParam1, const UByte inParam2, const UByte inParam3, const UByte inParam4, const UByte inParam5, const UByte inParam6);
	bool		IsValid (void) const;
	void		Clear (void);

} CC708WindowParms;


AJAExport std::ostream & operator << (std::ostream & inOutStream, const CC708WindowParms & inData);



//	708 Window Attributes - see CEA-708B, pg 48-49
//
enum
{
	NTV2_CC708JustifyMin	= 0,
	NTV2_CC708JustifyLeft	= NTV2_CC708JustifyMin,
	NTV2_CC708JustifyRight	= 1,
	NTV2_CC708JustifyCenter = 2,
	NTV2_CC708JustifyFull	= 3,
	NTV2_CC708JustifyMax	= NTV2_CC708JustifyFull
};


enum
{
	NTV2_CC708PrintDirMin	= 0,
	NTV2_CC708PrintDirLtoR	= NTV2_CC708PrintDirMin,
	NTV2_CC708PrintDirRtoL	= 1,
	NTV2_CC708PrintDirMax	= NTV2_CC708PrintDirRtoL
};


enum
{
	NTV2_CC708ScrollDirMin	= 0,
	NTV2_CC708ScrollDirLtoR	= NTV2_CC708ScrollDirMin,
	NTV2_CC708ScrollDirRtoL	= 1,
	NTV2_CC708ScrollDirTtoB	= 2,
	NTV2_CC708ScrollDirBtoT	= 3,
	NTV2_CC708ScrollDirMax	= NTV2_CC708ScrollDirBtoT
};


const bool NTV2_CC708WordWrap   = true;
const bool NTV2_CC708NoWordWrap = false;


enum
{
	NTV2_CC708DispEffectMin	 = 0,
	NTV2_CC708DispEffectSnap = NTV2_CC708DispEffectMin,
	NTV2_CC708DispEffectFade = 1,
	NTV2_CC708DispEffectWipe = 2,
	NTV2_CC708DispEffectMax	 = NTV2_CC708DispEffectWipe
};


enum
{
	NTV2_CC708EffectDirMin	= 0,
	NTV2_CC708EffectDirLtoR	= NTV2_CC708EffectDirMin,
	NTV2_CC708EffectDirRtoL	= 1,
	NTV2_CC708EffectDirTtoB	= 2,
	NTV2_CC708EffectDirBtoT	= 3,
	NTV2_CC708EffectDirMax	= NTV2_CC708EffectDirBtoT
};


enum
{
	NTV2_CC708BorderTypeMin			= 0,
	NTV2_CC708BorderTypeNone		= NTV2_CC708BorderTypeMin,
	NTV2_CC708BorderTypeRaised		= 1,
	NTV2_CC708BorderTypeDepressed	= 2,
	NTV2_CC708BorderTypeUniform		= 3,
	NTV2_CC708BorderTypeShdwLeft	= 4,
	NTV2_CC708BorderTypeShdwRight	= 5,
	NTV2_CC708BorderTypeMax			= NTV2_CC708BorderTypeShdwRight
};


typedef struct CC708WindowAttr
{
	int			justify;			// NTV2_CC708Justify___
	int			printDir;			// NTV2_CC708PrintDir___
	int			scrollDir;			// NTV2_CC708ScrollDir___
	bool		wordWrap;			// NTV2_CC708NoWordWrap/NTV2_CC708WordWrap
	int			displayEffect;		// NTV2_CC708DispEffect___
	int			effectDir;			// NTV2_CC708EffectDir___
	int			effectSpeed;
	int			borderType;			// NTV2_CC708BorderType___
	CC708Color	fillColor;
	CC708Color	borderColor;

	explicit	CC708WindowAttr ();
	explicit	CC708WindowAttr (const UByte inParam1, const UByte inParam2, const UByte inParam3, const UByte inParam4);
	bool		IsValid (void) const;
	void		Clear (void);

} CC708WindowAttr;


AJAExport std::ostream & operator << (std::ostream & inOutStream, const CC708WindowAttr & inData);


//	708 Pen Attributes - see CEA-708B, pg 50-51
//
enum
{
	NTV2_CC708PenSizeMin	  = 0,
	NTV2_CC708PenSizeSmall	  = NTV2_CC708PenSizeMin,
	NTV2_CC708PenSizeStandard = 1,
	NTV2_CC708PenSizeLarge	  = 2,
	NTV2_CC708PenSizeMax	  = NTV2_CC708PenSizeLarge
};


enum
{
	NTV2_CC708FontStyleMin				= 0,
	NTV2_CC708FontStyleUndefined		= NTV2_CC708FontStyleMin,
	NTV2_CC708FontStyleMonoSerif		= 1,
	NTV2_CC708FontStylePropSerif		= 2,
	NTV2_CC708FontStyleMonoSansSerif	= 3,
	NTV2_CC708FontStylePropSanSerif		= 4,
	NTV2_CC708FontStyleCasual			= 5,
	NTV2_CC708FontStyleCursive			= 6,
	NTV2_CC708FontStyleSmallCaps		= 7,
	NTV2_CC708FontStyleMax				= NTV2_CC708FontStyleSmallCaps
};


enum
{
	NTV2_CC708TextTagMin			= 0,
	NTV2_CC708TextTagDialog			= NTV2_CC708TextTagMin,
	NTV2_CC708TextTagSpeaker		= 1,
	NTV2_CC708TextTagElectronic		= 2,
	NTV2_CC708TextTagSecLanguage	= 3,
	NTV2_CC708TextTagVoiceover		= 4,
	NTV2_CC708TextTagAudibleTrans	= 5,
	NTV2_CC708TextTagSubtitleTrans	= 6,
	NTV2_CC708TextTagVoiceDesc		= 7,
	NTV2_CC708TextTagLyrics			= 8,
	NTV2_CC708TextTagSoundEfx		= 9,
	NTV2_CC708TextTagMusicScore		= 10,
	NTV2_CC708TextTagExpletive		= 11,
	NTV2_CC708TextTagReserved1		= 12,
	NTV2_CC708TextTagReserved2		= 13,
	NTV2_CC708TextTagReserved3		= 14,
	NTV2_CC708TextTagTextNoDisplay	= 15,
	NTV2_CC708TextTagMax			= NTV2_CC708TextTagTextNoDisplay
};


enum
{
	NTV2_CC708PenOffsetMin			= 0,
	NTV2_CC708PenOffsetSubscript	= NTV2_CC708PenOffsetMin,
	NTV2_CC708PenOffsetNormal		= 1,
	NTV2_CC708PenOffsetSuperscript	= 2,
	NTV2_CC708PenOffsetMax			= NTV2_CC708PenOffsetSuperscript
};


const bool NTV2_CC708Italics		(true);
const bool NTV2_CC708NoItalics		(false);

const bool NTV2_CC708Underline		(true);
const bool NTV2_CC708NoUnderline	(false);


enum
{
	NTV2_CC708PenEdgeTypeMin			 = 0,
	NTV2_CC708PenEdgeTypeNone			 = NTV2_CC708PenEdgeTypeMin,
	NTV2_CC708PenEdgeTypeRaised			 = 1,
	NTV2_CC708PenEdgeTypeDepressed		 = 2,
	NTV2_CC708PenEdgeTypeUniform		 = 3,
	NTV2_CC708PenEdgeTypeLeftDropShadow	 = 4,
	NTV2_CC708PenEdgeTypeRightDropShadow = 5,
	NTV2_CC708PenEdgeTypeMax			 = NTV2_CC708PenEdgeTypeRightDropShadow
};


typedef struct CC708PenAttr
{
	int			penSize;		// NTV2_CC708PenSize___
	int			fontStyle;		// NTV2_CC708FontStyle___
	int			textTag;		// NTV2_CC708TextTag___
	int			offset;			// NTV2_CC708PenOffset___
	bool		italics;		// NTV2_CC708NoItalics/NTV2_CC708Italics
	bool		underline;		// NTV2_CC708NoUnderline/NTV2_CC708Underline
	int			edgeType;		// NTV2_CC708PenEdgeType___

	explicit	CC708PenAttr ();											///< @brief	Default constructor
	explicit	CC708PenAttr (const UByte inParam1, const UByte inParam2);	///< @brief	Construct from two parameter bytes
	inline bool	IsValid (void) const	{return		penSize		>= NTV2_CC708PenSizeMin			&& penSize		<= NTV2_CC708PenSizeMax
												&&	fontStyle	>= NTV2_CC708FontStyleMin		&& fontStyle	<= NTV2_CC708FontStyleMax
												&&	textTag		>= NTV2_CC708TextTagMin			&& textTag		<= NTV2_CC708TextTagMax
												&&	offset		>= NTV2_CC708PenOffsetMin		&& offset		<= NTV2_CC708PenOffsetMax
												&&	edgeType	>= NTV2_CC708PenEdgeTypeMin		&& edgeType		<= NTV2_CC708PenEdgeTypeMax;}
	void		Clear (void);
} CC708PenAttr;


AJAExport std::ostream & operator << (std::ostream & inOutStream, const CC708PenAttr & inData);


//	708 Pen Color - see CEA-708B, pg 52
//
typedef struct CC708PenColor
{
	CC708Color	fg;				// pen foreground color/opacity
	CC708Color	bg;				// pen background color/opacity
	CC708Color	edge;			// pen edge color (opacity ignored)

	explicit	CC708PenColor ();
	explicit	CC708PenColor (const UByte inParam1, const UByte inParam2, const UByte inParam3);	///< @brief	Construct from three parameter bytes
	inline bool	IsValid (void) const	{return fg.IsValid () && bg.IsValid () && edge.IsValid ();}
	void		Clear (void);

} CC708PenColor;


AJAExport std::ostream & operator << (std::ostream & inOutStream, const CC708PenColor & inData);


//	708 Pen Location - see CEA-708B pg 53
//
typedef struct CC708PenLocation
{
	int		row;
	int		column;

	explicit	CC708PenLocation (const int inRow = 0, const int inColumn = 0);
	inline bool	IsValid (void) const	{return row >= 0 && row <= 14 && column >= 0 && column <= 41;}
	inline void Reset (void)			{row = column = 0;}

} CC708PenLocation;


AJAExport std::ostream & operator << (std::ostream & inOutStream, const CC708PenLocation & inData);


/**
	@brief	I build CEA-708 ("DTVCC") closed-captioning commands.
			The commands are built in my local buffer, and commands may be concatenated
			by tracking the "index" into my buffer. Once the command(s) have been built,
			my local buffer may be accessed and/or copied to retrieve the results.

			I have a set of methods for building SMPTE 334 Anc Packets using my "mAnc334Data" buffer.
			The main client method is MakeSMPTE334AncPacket, which assumes that a valid Caption Channel Packet
			has already been built in "mPacketData", copies it into a full SMPTE-334 Anc Packet, and returns
			a pointer to the "mAnc334Data" buffer.

			I have another set of primitive methods for building 708 captioning commands, building the results
			in my "mPacketData" buffer. The results can be retrieved using GetCaptionChannelPacket, and/or
			left in place for use by MakeSMPTE334AncPacket.

			To insert the SMPTE-334 Anc packet into a host video buffer (in the VANC lines), call my
			InsertSMPTE334AncPacketInVideoFrame method.
**/
class CNTV2CaptionEncoder708;
typedef AJARefPtr <CNTV2CaptionEncoder708>	CNTV2CaptionEncoder708Ptr;


class AJAExport CNTV2CaptionEncoder708 : public CNTV2CaptionLogConfig
{
	//	CLASS METHODS
	public:
		/**
			@brief		Creates a new CNTV2CaptionEncoder708 instance.
			@param[out]	outEncoder	Receives the newly-created encoder instance.
			@return		True if successful; otherwise False.
			@note		This method will catch any "bad_alloc" exception and return False
						if there's insufficient free memory to allocate the new encoder.
		**/
		static bool				Create (CNTV2CaptionEncoder708Ptr & outEncoder);


	//	INSTANCE METHODS
	public:
		virtual ~CNTV2CaptionEncoder708 ();

		virtual void	Reset (void);

		//	These methods build 708 messages in my mPacketData private buffer...
		virtual void	InitCaptionChannelPacket ();
		virtual bool	SetCaptionChannelPacket (const UBytePtr pInData, const size_t numBytes);
		virtual inline UBytePtr			GetCaptionChannelPacket (void)				{return mPacketData;}
		virtual inline const UByte *	GetCaptionChannelPacket (void) const		{return mPacketData;}
		virtual inline size_t			GetCaptionChannelPacketSize (void) const	{return mPacketDataSize;}
		virtual bool	SetCaptionChannelPacketSize (size_t packetSize);

		virtual bool	MakeCaptionChannelPacketHeader (const size_t index, size_t packetSize, size_t & outNewIndex);
		virtual bool	MakeNullServiceBlockHeader (size_t index, size_t & outNewIndex);
		virtual bool	MakeServiceBlockHeader (const size_t index, int serviceNum, const size_t blockSize, size_t & outNewIndex);
		virtual bool	MakeServiceBlockHeader (const size_t index, int serviceNum, const size_t blockSize);
		virtual bool	MakeServiceBlockCharData (const size_t index, UByte data, size_t & outNewIndex);

		virtual bool	MakeDefineWindowCommand (const size_t index, int windowID, const CC708WindowParms & inParms, size_t & outNewIndex);
		virtual bool	MakeClearWindowsCommand (const size_t index, UByte windowMap, size_t & outNewIndex);
		virtual bool	MakeDeleteWindowsCommand (const size_t index, UByte windowMap, size_t & outNewIndex);
		virtual bool	MakeDisplayWindowsCommand (const size_t index, UByte windowMap, size_t & outNewIndex);
		virtual bool	MakeHideWindowsCommand (const size_t index, UByte windowMap, size_t & outNewIndex);
		virtual bool	MakeToggleWindowsCommand (const size_t index, UByte windowMap, size_t & outNewIndex);
		virtual bool	MakeSetCurrentWindowCommand (const size_t index, const int windowID, size_t & outNewIndex);
		virtual bool	MakeSetWindowAttributesCommand (const size_t index, const CC708WindowAttr & inAttr, size_t & outNewIndex);

		virtual bool	MakeSetPenAttributesCommand (const size_t index, const CC708PenAttr & inAttr, size_t & outNewIndex);
		virtual bool	MakeSetPenColorCommand (const size_t index, const CC708PenColor & inColor, size_t & outNewIndex);
		virtual bool	MakeSetPenLocationCommand (const size_t index, const CC708PenLocation & inLoc, size_t & outNewIndex);

		virtual bool	MakeDelayCommand (const size_t index, const UByte delay, size_t & outNewIndex);
		virtual bool	MakeDelayCancelCommand (const size_t index, size_t & outNewIndex);
		virtual bool	MakeResetCommand (const size_t index, size_t & outNewIndex);


		//	These methods build SMPTE 334 messages in my mAnc334Data private buffer...

		/**
			@brief		Clears my private CEA-608 caption data buffer.
			@return		True if successful; otherwise false.
		**/
		virtual bool	Clear608CaptionData (void);

		/**
			@brief		Sets the CEA-608 caption data in my private 608 data buffer.
			@param[in]	inCC608Data		Specifies the caption data bytes to be stored in my private buffer.
			@note		The caption bytes must have the standard CEA-608 odd parity.
			@return		True if successful; otherwise false.
		**/
		virtual bool	Set608CaptionData (const CaptionData & inCC608Data);
		/**
			@brief		Sets my CEA-608 caption data for the given field.
			@param[in]	inField		Specifies the field.
			@param[in]	inChar1		Specifies the first caption data byte. (Must include odd parity.)
			@param[in]	inChar2		Specifies the second caption data byte. (Must include odd parity.)
			@param[in]	inGotData	Specifies whether or not the specified data bytes are valid.
			@return		True if successful; otherwise false.
		**/
		virtual bool	Set608CaptionData (const NTV2Line21Field inField, const UByte inChar1, const UByte inChar2, const bool inGotData);

		/**
			@brief		Generates a SMPTE-334 Ancillary data packet.
			@param[in]	inFrameRate			Specifies the frame rate of the outgoing video into which the
											resulting ancillary data packet(s) will be inserted.
			@param[in]	inVideoField		Specifies the video field.
			@return		True if successful; otherwise false.
			@note		For ::NTV2_FRAMERATE_2398, ::NTV2_FRAMERATE_2400, ::NTV2_FRAMERATE_2997 and ::NTV2_FRAMERATE_3000,
						the \c inVideoField parameter is ignored, and whatever ::CaptionData for fields F1 and/or F2 is valid
						(as set in an earlier call to CNTV2CaptionEncoder708::Set608CaptionData) will be inserted into the packet.
		**/
		virtual bool	MakeSMPTE334AncPacket (const NTV2FrameRate inFrameRate, const NTV2Line21Field inVideoField);

		/**
			@brief		Generates a SMPTE-334 Ancillary data packet.
			@param[in]	inFrameRate			Specifies the frame rate of the outgoing video into which the
											resulting ancillary data packet(s) will be inserted.
			@param[in]	inVideoField		Specifies the video field.
			@param[out]	outAncPacketData	Specifies a variable that is to receive a pointer to my internal
											buffer that contains the generated caption data packet (CDP).
			@param[out]	outSize				Specifies a variable that is to receive the number of bytes of CDP
											data that got placed into my internal buffer.
			@return		True if successful; otherwise false.
		**/
		virtual bool	MakeSMPTE334AncPacket (const NTV2FrameRate inFrameRate, const NTV2Line21Field inVideoField, UWordPtr & outAncPacketData, size_t & outSize);

		/**
			@brief		Generates a SMPTE 334 Ancillary Packet using a supplied CDP.
			@param[in]	pInCDP				Specifies a valid, non-NULL pointer to a buffer that contains the CDP to be used.
			@param[in]	inCDPLength			Specifies the length of the specified CDP, in bytes.
			@param[out]	outAncPacketData	Receives a pointer to my internal buffer that contains the generated packet.
			@param[out]	outSize				Receives the number of bytes of packet data that got placed into my internal buffer.
			@note		This function assumes that the supplied 8-bit CDP is correct and complete, and only needs parity added.
			@return		True if successful; otherwise false.
		**/
		virtual bool	MakeSMPTE334AncPacketFromCDP (const UBytePtr pInCDP, const size_t inCDPLength, UWordPtr & outAncPacketData, size_t & outSize);

		/**
			@brief		Generates a SMPTE 334 Ancillary Packet using a supplied CDP.
			@param[in]	pInCDP				Specifies a valid, non-NULL pointer to a buffer that contains the CDP to be used.
			@param[in]	inCDPLength			Specifies the length of the specified CDP, in bytes.
			@note		This function assumes that the supplied 8-bit CDP is correct and complete, and only needs parity added.
			@return		True if successful; otherwise false.
		**/
		virtual bool	MakeSMPTE334AncPacketFromCDP (const UBytePtr pInCDP, const size_t inCDPLength);

		virtual bool	SetServiceInfoActive (int svcIndex, bool bActive);
		virtual bool	CopyAllServiceInfo (const NTV2_CC708ServiceData & inSrcSvcData);

		/**
			@brief		Inserts the encoder’s most recently generated ancillary data packet into the given host frame buffer.
			@param		pFrameBuffer		Specifies a valid, non-NULL starting address of the host frame buffer.
			@param[in]	inVideoFormat		Specifies the video format of the host frame buffer.
			@param[in]	inPixelFormat		Specifies the pixel format of the host frame buffer.
			@param[in]	inVancLineNumber	Specifies the line number in the host frame buffer into which the anc data will be embedded.
											Note that this is not the same as the SMPTE line number, which, by convention, for SMPTE 334,
											is line 9. Use the CNTV2SMPTEAncData::GetVancLineOffset class method to calculate the correct
											frame buffer line number (i.e., the "VANC" line number).
			@param[in]	inWordOffset		Specifies the two-byte word offset in the host buffer line into which the ancillary data will
											be inserted. Defaults to 1, which will embed the anc data into the luma channel.
			@return		True if successful; otherwise false.
		**/
		virtual bool	InsertSMPTE334AncPacketInVideoFrame (void * pFrameBuffer,
															const NTV2VideoFormat inVideoFormat,
															const NTV2FrameBufferFormat inPixelFormat,
															const ULWord inVancLineNumber,
															const ULWord inWordOffset = 1) const;

		//	DEBUG methods
		virtual NTV2CaptionLogMask	SetLogMask (const NTV2CaptionLogMask inLogMask);
		virtual inline void			Set608TestIDMode (const bool inEnableTestIDMode)			{ mFlip608Characters = inEnableTestIDMode; }

		virtual UWordSequence		GetSMPTE334DataVector (void) const;
		virtual const UWord *		GetSMPTE334Data (void) const								{return mAnc334Data;}
		virtual size_t				GetSMPTE334Size (void) const								{return mAnc334Size;}


	//	Private Instance Methods
	private:
		virtual bool	InsertSMPTE334AncHeader	(size_t & inOutAncIndex, const UByte inDataCount);
		virtual bool	InsertSMPTE334AncFooter	(size_t & inOutAncIndex, const size_t inAncStartIndex);
		virtual bool	InsertCDPHeader			(size_t & inOutAncIndex, const NTV2FrameRate inFrameRate, const int cdpSeqNum);
		virtual bool	InsertCDPFooter			(size_t & inOutAncIndex, const size_t cdpStartIndex, const int cdpSeqNum);
		virtual bool	InsertCDPData			(size_t & inOutAncIndex, const NTV2FrameRate inFrameRate, const NTV2Line21Field inVideoField);
		virtual bool	InsertCDPDataTriplet	(size_t & inOutAncIndex, const bool ccValid, const int ccType, const UByte data1, const UByte data2);
		virtual bool	InsertCDPServiceInfo	(size_t & inOutAncIndex);

		explicit								CNTV2CaptionEncoder708 ();
		explicit								CNTV2CaptionEncoder708 (const CNTV2CaptionEncoder708 & inEncoderToCopy);
		virtual inline CNTV2CaptionEncoder708 &	operator = (const CNTV2CaptionEncoder708 & inEncoderToCopy)					{(void) inEncoderToCopy; return *this;}


	//	Private Class Methods
	private:
		static bool		ConvertFrameRate (const NTV2FrameRate inNTV2FrameRate, size_t & outConverted, size_t & outNumCCPackets, size_t & outNum608Triplets);
		static UByte	CC608OddParity (UByte c);


	//	Instance Data
	private:
		CaptionData	m608CaptionData;					///< @brief	Used to fill the "NTSC" sections of the output
		UByte		mPacketData [NTV2_CC708MaxPktSize];	///< @brief	Storage for building my CDP packet data
		size_t		mPacketDataSize;					///< @brief	Current number of bytes in the CDP packet
		int			mPacketSequenceNum;					///< @brief	Caption Channel Packet Sequence count (mod 4)
		UWord		mAnc334Data [NTV2_CC708MaxAncSize];	///< @brief	Storage for building my SMPTE 334 Ancillary Packet
		size_t		mAnc334Size;						///< @brief	Current number of words in the SMPTE 334 Ancillary packet
		UWord		mCDPSequenceNum;					///< @brief	Rolling count used to identify sequences
		CNTV2Caption708ServiceInfo	mServiceInfo;		///< @brief	CaptionServiceInfo object holding current service info
		int			mNumServiceInfoPerCDP;				///< @brief	Maximum number of service_info thingies I'm willing to output in a single CDP
		bool		mFlip608Characters;					///< @brief	For test/ID purposes: set 'true' to flip the case of 608 stream chars

};	//	CNTV2CaptionEncoder708

#endif	// __NTV2_CEA708_ENCODER_
