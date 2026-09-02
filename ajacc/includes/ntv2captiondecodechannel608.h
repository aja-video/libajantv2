/**
	@file		ntv2captiondecodechannel608.h
	@brief		Declares the CNTV2CaptionDecodeChannel608 class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CEA608_DECODECHANNEL_
#define __NTV2_CEA608_DECODECHANNEL_

#include "ntv2captionlogging.h"
#include "ntv2caption608types.h"
#include "ajabase/common/ajarefptr.h"
#include "ajabase/system/lock.h"
#include <vector>

#ifdef MSWindows
	#include "windows.h"
	#include "stdio.h"
#endif


/**
	@brief	The currently supported caption decoder stats.
**/
typedef enum _CaptionDecode608Stats
{
	kParsedOKTally		= 0,	///< @brief	Total Parse608Data successes
	kParseFailTally		= 1,	///< @brief	Total Parse608Data failures
	kTotalTabOffsetCmds	= 2,	///< @brief	Total tab offset commands parsed
	kTotalCharSetCmds	= 3,	///< @brief	Total character set commands parsed
	kTotalAttribCmds	= 4,	///< @brief	Total attribute commands parsed
	kTotalMiscCmds		= 5,	///< @brief	Total Misc commands parsed (RollUp, FlashOn, RCL, BS, EDM, EOC, etc.)
	kTotalPACCmds		= 6,	///< @brief	Total PAC (preamble address code) commands parsed
	kTotalMidRowCmds	= 7,	///< @brief	Total Mid-Row commands parsed (color, underline, italic, flash, etc.)
	kTotalSpecialChars	= 8,	///< @brief	Total special characters parsed
	kTotalCharData		= 9,	///< @brief	Total plain ol' characters parsed
	kMaxTallies			= 16
} CaptionDecode608Stats;



/**
	@brief	I decode a single channel of CEA-608 ("Line 21") closed captioning.
			CEA-608 allows for up to eight "captioning channels" (CC1 - CC4, Text1 - Text4) that can be active
			and receiving data at a given time. My clients will typically parse the caption data stream and
			decide which channel to send the data to. My methods should only be called with data that is
			pertinent to the channel it's been asked to decode.

			My principal high-level method is CNTV2CaptionDecodeChannel608::Parse608Data, which takes two bytes
			of data (the most any one channel can receive in a video frame), and decodes it. I maintain the
			character buffers and the current state of the channel, which clients may query at will.

			Eight instances of me -- one per CEA-608 caption channel -- are used by each
			CNTV2CaptionDecodeChannel608::CNTV2CaptionDecode608 instance, and (using the
			CNTV2CaptionTranslatorChannel608to708 subclass) CNTV2CaptionTranslator608to708.
			Clients will typically work with these higher-level objects to implement a full decoder and/or translator.

	@note	The CNTV2CaptionTranslatorChannel608to708 class is derived from me, which adds functionality needed to
			translate CEA-608 captions to CEA-708 format. Any changes made to me could affect the subclass.
**/
class CNTV2CaptionDecodeChannel608;
typedef AJARefPtr <CNTV2CaptionDecodeChannel608>	CNTV2CaptionDecodeChannel608Ptr;

class AJAExport CNTV2CaptionDecodeChannel608 : public CNTV2CaptionLogConfig
{
	//	CLASS METHODS
	public:
		/**
			@brief	Creates and returns a new CNTV2CaptionDecodeChannel608 instance.
			@param[out]	outInstance		Receives the newly-created CNTV2CaptionDecodeChannel608 instance.
			@return		True if successful; otherwise false.
		**/
		static bool						Create (CNTV2CaptionDecodeChannel608Ptr & outInstance);


	//	INSTANCE METHODS
	public:
		/**
			@name	Decoder Configuration
		**/
		///@{
		/**
			@brief		Restores my state -- caption channel CC1, screen 0, cursor at row 15 column 1, pop-on mode,
						default CEA-608 character set, roll-up base at row 15, and a 15-row Text Mode.
		**/
		virtual void					Reset (void);

		/**
			@brief		Changes the caption channel that I handle.
			@param[in]	inNewChannel	Specifies the caption channel I'll be handling.
			@return		True if successful;  otherwise false.
		**/
		virtual bool					SetChannel (const NTV2Line21Channel inNewChannel);

		/**
			@brief		Answers true if the caption channel I'm currently handling is Tx1, Tx2, Tx3 or Tx4.
			@return		True if I'm currently handling one of the Text Mode caption channels;  otherwise false.
		**/
		virtual inline bool				IsTextChannel (void) const										{return IsLine21TextChannel (mChannel);}

		/**
			@brief		Answers with my current character set.
			@return		My current character set.
		**/
		virtual inline NTV2Line21CharacterSet	GetCurrentCharacterSet (void) const						{return mCharacterSet;}

		virtual NTV2Line21Channel		GetCurrentChannel (UByte char608_1, UByte char608_2, NTV2Line21Field field);
		///@}

		/**
			@name	Decoder Operation
		**/
		///@{
		/**
			@brief		This method pushes caption data into me. It should be called when two new bytes of CEA-608
						caption data arrive for this channel. The caller should have already filtered out any
						"duplicate" commands (i.e. commands that are sent twice on adjacent frames) before calling
						this method. There's no need to call this if there's no new data for my channel.
			@param[in]	inByte1		The first caption data byte. The parity bit must have already been stripped off.
			@param[in]	inByte2		The second caption data byte. The parity bit must have already been stripped off.
			@param[out]	outDebugStr	Receives a string that contains some useful information if this call fails.
			@return		True if successful;  otherwise false.
		**/
		virtual bool					Parse608Data (const UByte inByte1, const UByte inByte2, std::string & outDebugStr);

		/**
			@brief		Answers with the row at which the next caption character will be inserted.
			@return		My current row position, a value from 1 to 15.
		**/
		virtual inline UWord			GetRow (void) const												{return mRow;}

		/**
			@brief		Answers with the column at which the next caption character will be inserted.
			@return		My current column position, a value from 1 to 32.
		**/
		virtual inline UWord			GetColumn (void) const											{return mCol;}
		///@}

		/**
			@name	Character Screen Back-Buffer Access
		**/
		///@{
		/**
			@brief		Returns the "ASCII-like" character code at the given screen position (including its attributes).
			@param[in]	inRow		The row number (1-15).
			@param[in]	inCol		The column number (1-32).
			@param[out]	outAttr		An NTV2Line21Attributes variable that is to receive the display attributes of the on-screen character.
			@return		The character code of the character at the given screen position.
		**/
		virtual UByte					GetOnAirCharacter (const UWord inRow, const UWord inCol, NTV2Line21Attrs & outAttr) const;

		/**
			@brief		Returns a string containing the UTF-8 character sequence that best represents the caption character
						at the given screen position.
			@param[in]	inRow		The row number (1-15).
			@param[in]	inCol		The column number (1-32).
			@return		A string that contains the UTF8 character sequence that best represents the on-screen caption character.
						The returned string will be empty upon failure, or if there is no best representation for the character.
		**/
		virtual std::string				GetOnAirCharacter (const UWord inRow, const UWord inCol) const;

		/**
			@brief		Returns a string containing the UTF-8 character sequence that best represents the caption character at
						the given screen position.
			@param[in]	inRow		The row number (1-15).
			@param[in]	inCol		The column number (1-32).
			@param[out]	outAttr		An NTV2Line21Attributes variable that is to receive the display attributes of the on-screen character.
			@return		A string that contains the UTF8 character sequence that best represents the on-screen caption character.
						The returned string will be empty upon failure, or if there is no best representation for the character.
		**/
		virtual std::string				GetOnAirCharacterWithAttributes (const UWord inRow, const UWord inCol, NTV2Line21Attrs & outAttr) const;

		/**
			@brief		Returns the UTF-16 character that best represents the caption character at the given screen position.
			@param[in]	inRow		The row number (1-15).
			@param[in]	inCol		The column number (1-32).
			@param[out]	outAttr		Receives the NTV2Line21Attributes of the on-screen character.
			@return		The UTF-16 character that best represents the on-screen caption character, or zero upon failure.
		**/
		virtual UWord					GetOnAirUTF16CharacterWithAttributes (const UWord inRow, const UWord inCol, NTV2Line21Attrs & outAttr) const;

		/**
			@brief		Returns the NTV2Line21CharacterSet of the caption character that's currently at the given screen position.
			@param[in]	inRow		The row number (1-15).
			@param[in]	inCol		The column number (1-32).
			@return		The NTV2Line21CharacterSet of the caption character that's currently at the given screen position.
		**/
		virtual NTV2Line21CharacterSet	GetOnAirCharacterSet (const UWord inRow, const UWord inCol) const;

		virtual bool					GetCharacterAttributes (const UWord screen, const UWord inRow, const UWord inCol, NTV2Line21Attrs & outAttr) const;
		///@}

		/**
			@name	Text Mode Configuration
		**/
		///@{
		/**
			@brief		Returns the number of rows used for displaying Text Mode captions (Tx1/Tx2/Tx3/Tx4).
			@return		The number of rows used for displaying Text Mode captions.
		**/
		virtual inline UWord			GetTextModeDisplayRowCount (void) const										{return mTextRows;}

		/**
			@brief		Changes the number of rows used for displaying Text Mode captions (Tx1/Tx2/Tx3/Tx4).
			@param[in]	inNumRows	Specifies the number of rows to use for displaying Text Mode captions.
									Must be at least 1 and no more than 32.
			@return		True if successful;  otherwise false.
		**/
		virtual bool					SetTextModeDisplayRowCount (const UWord	inNumRows);

		/**
			@brief		Returns the display attributes that Text Mode captions are currently using (assuming my caption
						channel is Tx1/Tx2/Tx3/Tx4).
			@return		My current Text Mode caption display attributes.
		**/
		virtual inline const NTV2Line21Attrs &		GetTextModeDisplayAttributes (void) const						{return mTextModeStyle;}

		/**
			@brief		Sets the display attributes that Text Mode captions will use henceforth.
			@param[in]	inAttributes	Specifies the new display attributes for Text Mode captions will have going forward.
			@note		This has no effect on captions that have already been decoded (that may be currently displayed).
		**/
		virtual inline void				SetTextModeDisplayAttributes (const NTV2Line21Attrs & inAttributes)			{mTextModeStyle = inAttributes;}
		///@}

		/**
			@name	Debug
		**/
		///@{
		virtual std::string				GetDebugPrintRow (const UWord inRow, const bool inShowChars = true) const;
		virtual std::string				GetDebugPrintScreen (const bool inShowChars = true) const;
		virtual void					SetDebugRowsOfInterest (const UWord inFromRow, const UWord inToRow, const bool inAdd = false);
		virtual void					SetDebugColumnsOfInterest (const UWord inFromCol, const UWord inToCol, const bool inAdd = false);
		///@}

		virtual							~CNTV2CaptionDecodeChannel608 ();

		virtual std::vector<uint32_t>	GetStats(void) const;
		static std::string				GetStatTitle(const CaptionDecode608Stats inStat);
		virtual bool					SubscribeChangeNotification (NTV2Caption608Changed * pInCallback, void * pInUserData = NULL);
		virtual bool					UnsubscribeChangeNotification (NTV2Caption608Changed *	pInCallback, void * pInUserData = NULL);


	//	PROTECTED INSTANCE METHODS
	protected:
		virtual bool 					Parse608CharacterData		(UByte char608_1, UByte char608_2, std::string & outDebugStr);
		virtual bool					Parse608TabOffsetCommand	(UByte char608_1, UByte char608_2, std::string & outDebugStr);
		virtual bool					Parse608CharacterSetCommand	(UByte char608_1, UByte char608_2, std::string & outDebugStr);
		virtual bool					Parse608AttributeCommand	(UByte char608_1, UByte char608_2, std::string & outDebugStr);
		virtual bool					Parse608PACCommand			(UByte char608_1, UByte char608_2, std::string & outDebugStr);
		virtual bool					Parse608MidRowCommand		(UByte char608_1, UByte char608_2, std::string & outDebugStr);
		virtual bool					Parse608SpecialCharacter	(UByte char608_1, UByte char608_2, std::string & outDebugStr);
		virtual bool					Parse608MiscCommand			(UByte char608_1, UByte char608_2, std::string & outDebugStr);
		
		virtual bool					DoResumeCaptionLoading		(void);
		virtual bool					DoBackspace					(void);
		virtual bool					DoDeleteToEndOfRow			(void);
		virtual bool					DoRollUpCaption				(const UWord inRows);
		virtual bool					DoFlashOn					(void);
		virtual bool					DoResumeDirectCaptioning	(void);
		virtual bool					DoTextRestart				(void);
		virtual bool					DoResumeTextDisplay			(void);
		virtual bool					DoEraseDisplayedMemory		(void);
		virtual bool					DoCarriageReturn			(void);
		virtual bool					DoEraseNonDisplayedMemory	(void);
		virtual bool					DoEndOfCaption				(void);

	private:
		//	Low-level back buffer accessors
		virtual bool					SetScreenCharacter			(const UWord inScreenNum, const UWord inRow, const UWord inCol, const NTV2_CC608_CodePoint inNewCodePoint);
		virtual NTV2_CC608_CodePoint	GetScreenCharacter			(const UWord inScreenNum, const UWord inRow, const UWord inCol) const;
		virtual bool					SetScreenAttributes			(const UWord inScreenNum, const UWord inRow, const UWord inCol, const NTV2Line21Attrs inAttr);
		virtual NTV2Line21Attrs			GetScreenAttributes			(const UWord inScreenNum, const UWord inRow, const UWord inCol) const;

	protected:
		//	Higher-level accessors
		virtual void					SetAttributes				(const UWord inRow, const UWord inCol, const NTV2Line21Attrs inAttr);
		virtual bool					EraseScreen					(const UWord screen);

		virtual bool					SetRow						(const UWord inNewRow);
		virtual bool					IncrementRow				(const int inDelta = 1);

		virtual bool					SetColumn					(const UWord inNewCol);
		virtual bool					IncrementColumn				(const int inDelta = 1);
		
		virtual bool					SetCaptionMode				(const NTV2Line21Mode inNewCaptionMode);
		virtual inline NTV2Line21Mode	GetCaptionMode				(void) const					{return mCaptionMode;}

		virtual inline UWord			GetCurrentScreen			(void) const					{return mCurrScreen;}
		virtual bool					SetCurrentScreen			(const UWord inNewScreen);

		virtual void					MoveRollUpWindow			(const UWord inNewBaseRow);

		virtual void					InsertCharacter				(const UByte char608_1, const UByte char608_2, const NTV2Line21Attrs inAttr);
		virtual void					InsertCharacter				(const UByte char608_1, const UByte char608_2);
		virtual void					InsertTextCharacter			(const UByte inASCIIChar);

		//	Someday we may want to signal interested third parties when my state changes:
		virtual void					Notify_ChannelChanged		(const NTV2Line21Channel inOldChannel, const NTV2Line21Channel inNewChannel) const;
		virtual void					Notify_CurrentRowChanged	(const UWord inOldRow, const UWord inNewRow) const;
		virtual void					Notify_CurrentColumnChanged	(const UWord inOldCol, const UWord inNewCol) const;
		virtual void					Notify_CurrentScreenChanged	(const UWord inOldScreen, const UWord inNewScreen) const;
		virtual void					Notify_CaptionModeChanged	(const NTV2Line21Mode inOldMode, const NTV2Line21Mode inNewMode) const;
		virtual void					Notify_ScreenCharChanged	(const UWord inScreenNum, const UWord inRow, const UWord inCol,
																	 const NTV2_CC608_CodePoint inOldCodePoint, const NTV2_CC608_CodePoint inNewCodePoint) const;
		virtual void					Notify_ScreenAttrChanged	(const UWord inScreenNum, const UWord inRow, const UWord inCol,
																	 const NTV2Line21Attrs & inOldAttr, const NTV2Line21Attrs & inNewAttr) const;


	//	PRIVATE INSTANCE METHODS
	protected:
		typedef NTV2_CC608_CodePoint	NTV2CC608CodePoint;
		explicit								CNTV2CaptionDecodeChannel608 ();	//	Hidden constructor
	private:
		explicit								CNTV2CaptionDecodeChannel608 (const CNTV2CaptionDecodeChannel608 & inObj);
		virtual CNTV2CaptionDecodeChannel608 &	operator = (const CNTV2CaptionDecodeChannel608 & inObj);


	//	INSTANCE DATA
	private:
		NTV2CC608CodePoint	mScreen	[2][NTV2_CC608_MaxRow+1][NTV2_CC608_MaxCol+1];	///< @brief	Two 32x15 "screens" of captioning characters per channel.
		NTV2Line21Attrs		mAttrs	[2][NTV2_CC608_MaxRow+1][NTV2_CC608_MaxCol+1];	///< @brief	Because captioning specs are "1-based", column 0 and row 0
																					///<		are not used. At any given time, one screen is on-air,
																					///<		the other is not.
		mutable void *		mpScreenLock;		///< @brief	Protects my backbuffers from simultaneous access by more than one thread
		UWord				mCurrScreen;		///< @brief	The index (0 or 1) of current on-air "screen".
		UWord				mRow;				///< @brief	The row of the current "cursor", or character insert point.
		UWord				mCol;				///< @brief	The column of the current "cursor", or character insert point.
		UWord				mRollBaseRow;		///< @brief	The "base row" used for RollUp mode
		UWord				mTextRows;			///< @brief	Number of displayed rows in Text Mode
		NTV2Line21Attrs		mTextModeStyle;		///< @brief	Character style to use in Text Mode
		NTV2Line21Mode		mCaptionMode;		///< @brief	Current caption mode (roll-up, pop-on, etc.)
		NTV2Line21Channel	mChannel;			///< @brief	My caption channel
		NTV2Line21CharSet	mCharacterSet;		///< @brief	My character set

		//	Debugging/logging:
		Line21RowSet			mDebugRows;		///< @brief	Specifies rows-of-interest for debugging
		Line21ColumnSet			mDebugCols;		///< @brief	Specifies columns-of-interest for debugging

		NTV2Caption608Changed *	mpCallback;		///< @brief	Change notification callback, if any
		void *					mpUserData;		///< @brief	Change notification user data, if any

		//	Stats
		std::vector<uint32_t>	mStats;			///< @brief	Stats -- see CaptionDecode608Stats enums for semantics
		mutable AJALock			mStatsLock;		///< @brief	Guard mutex for mStats

	public:											//	BACK-BUFFER TEST MODE
		static bool		sStaticBackBufferTestMode;	//	Defaults to false.
													//	To test me with static back-buffers, set sStaticBackBufferTestMode
													//	true, then call Reset, then operate me normally.
													//	In this mode, GetScreenCharacter and GetScreenAttributes will always
													//	return the fixed test pattern that's set in my constructor.

#if defined(AJA_DEBUG)								//	DEBUG BUILDS ONLY:	ClassTest and InstanceTest
		static	bool	ClassTest (void);			//	ClassTest:		Implementation in libajacc/test/main.cpp
		bool			InstanceTest (void);		//	InstanceTest:	Implementation in libajacc/test/main.cpp
#endif	//	AJA_DEBUG

};	//	CNTV2CaptionDecodeChannel608


AJA_EXPORT bool		Check608Parity (UByte char608_1, UByte char608_2, std::string & outDebugStr);
AJA_EXPORT UByte	Add608OddParity (UByte ch);

#endif	// __NTV2_CEA608_DECODECHANNEL_
