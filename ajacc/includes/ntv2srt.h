/**
	@file		ntv2srt.h
	@brief		Declares SubRipText codec classes.
	@copyright	(C) 2021-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_SRT_H_
#define __NTV2_SRT_H_

	#include "ajatypes.h"
	#include <vector>
	#include <map>
	#include <string>
	#include <iostream>

	typedef uint32_t	SRTTimestamp;	//	SRT timestamp, in milliseconds
	typedef uint32_t	SRTDuration;	//	SRT time duration, in milliseconds
	typedef uint32_t	SRTSeqNum;		//	SRT caption sequence number
	typedef uint16_t	SRTAnchorPos;	//	SRT caption anchor position


	class SRTWindow
	{
		public:
			inline				SRTWindow (uint32_t inX1 = 0,  uint32_t inY1 = 0,
											uint32_t inX2 = 0,  uint32_t inY2 = 0)
															{set(inX1, inY1, inX2, inY2);}
			//	Inquiry
			inline uint32_t		top (void) const			{return mY1;}
			inline uint32_t		left (void) const			{return mX1;}
			inline uint32_t		bottom (void) const			{return mY2;}
			inline uint32_t		right (void) const			{return mX2;}
			inline uint32_t		height (void) const			{return bottom() > top() ? bottom() - top() : 0;}
			inline uint32_t		width (void) const			{return right() > left() ? right() - left() : 0;}
			inline bool			isValid (void) const		{return height()  &&  width();}
			inline bool			isEmpty (void) const		{return (height() == 0)  ||  (width() == 0);}
			inline bool			operator == (const SRTWindow & inRHS) const	{return this == &inRHS  ||  (mX1 == inRHS.mX1 && mX2 == inRHS.mX2 && mY1 == inRHS.mY1 && mY2 == inRHS.mY2);}
			inline bool			operator != (const SRTWindow & inRHS) const	{return !(*this == inRHS);}
			std::string			asString (void) const;
			std::ostream &		print (std::ostream & oss) const;

			//	Modify
			inline void			clear (void)				{mX1 = mY1 = mX2 = mY2 = 0;}
			inline SRTWindow &	setTop (uint32_t inY1)		{mY1 = inY1;  return *this;}
			inline SRTWindow &	setLeft (uint32_t inX1)		{mX1 = inX1;  return *this;}
			inline SRTWindow &	setBottom (uint32_t inY2)	{mY2 = inY2;  return *this;}
			inline SRTWindow &	setRight (uint32_t inX2)	{mX2 = inX2;  return *this;}
			inline SRTWindow &	setTopLeft (uint32_t inX1,  uint32_t inY1)		{setTop(inY1); return setLeft(inX1);}
			inline SRTWindow &	setBottomRight (uint32_t inX2,  uint32_t inY2)	{setBottom(inY2); return setRight(inX2);}
			inline SRTWindow &	set (uint32_t inX1,  uint32_t inY1,  uint32_t inX2,  uint32_t inY2)
															{setTopLeft(inX1,inY1); return setBottomRight(inX2,inY2);}
		private:
			uint32_t	mX1, mX2, mY1, mY2;	//	Window rectangle bounds
	};	//	SRTWindow

	inline std::ostream & operator << (std::ostream & inOutStrm,  const SRTWindow & inWindow)	{return inWindow.print(inOutStrm);}

	/**
		@brief	Detailed caption information.
	**/
	class SRTCaptionInfo
	{
		public:
			inline explicit				SRTCaptionInfo (const SRTSeqNum inSeqNum = 0,  const SRTTimestamp inStartTime = 0,
														const SRTDuration inDuration = 0,  const std::string & inRawCaption = std::string(),
														const size_t inStartLineNum = 0, const size_t inEndLineNum = 0)
																	{set(inSeqNum, inStartTime, inDuration, inRawCaption, inStartLineNum, inEndLineNum);}
			//	Inquiry
			std::string					caption (void) const;
			inline const std::string &	rawCaption (void) const			{return mRawCaption;}
			inline SRTSeqNum			sequenceNum (void) const		{return mSequenceNum;}
			inline SRTTimestamp			startTime (void) const			{return mStartTime;}
			inline SRTDuration			duration (void) const			{return mDuration;}
			inline SRTTimestamp			endTime (void) const			{return startTime() + duration();}
			inline const SRTWindow &	window (void) const				{return mWindow;}
			inline bool					hasWindow (void) const			{return mWindow.isValid();}
			inline uint16_t				anchor (void) const				{return mAnchorPos;}
			inline bool					hasAnchor (void) const			{return anchor();}
			inline bool					isValid (void) const			{return !caption().empty()  &&  sequenceNum()  &&  startTime();}
			inline bool					hasAnchorTag (void) const		{return rawCaption().find("{\\an") == 0;}
			inline size_t				startLineNumber (void) const	{return mStartLineNum;}
			inline size_t				endLineNumber (void) const		{return mEndLineNum;}
			inline bool					hasLineNumbers (void) const		{return startLineNumber() && endLineNumber();}
			inline size_t				startLineOffset (void) const	{return hasLineNumbers() ? startLineNumber()-1 : 0;}
			inline size_t				endLineOffset (void) const		{return hasLineNumbers() ? endLineNumber()-1 : 0;}
			inline bool					isFirstCaption (void) const		{return mFirstCaption;}
			bool						getCharWithAttrs (const size_t inPos, std::string & outChar, bool & outIsBold, bool & outIsItalic, std::string & outColor) const;

			//	Modifying
			inline SRTCaptionInfo &		reset (void)				{return set(0, 0, 0);}
			inline SRTCaptionInfo &		set (const SRTSeqNum inSeqNum, const SRTTimestamp inStartTime,
											const SRTDuration inDuration, const std::string & inRawCaption = std::string(),
											const size_t inStartLineNum = 0, const size_t inEndLineNum = 0)
											{	mSequenceNum = inSeqNum;  mStartTime = inStartTime;
												mDuration = inDuration;  mRawCaption = inRawCaption;
												mStartLineNum = inStartLineNum;  mEndLineNum = inEndLineNum;
												mAnchorPos = 0; mWindow.clear();  mFirstCaption = false;
												return *this;
											}
			inline SRTCaptionInfo &		setFirstCaption (const bool inIsFirst = true)	{mFirstCaption = inIsFirst;  return *this;}
			inline SRTCaptionInfo &		setWindow (const SRTWindow & inWindow)			{mWindow = inWindow;  return *this;}
			inline SRTCaptionInfo &		setAnchorPos (const uint16_t inAnchorPos)		{mAnchorPos = inAnchorPos;  return *this;}
			inline SRTCaptionInfo &		setLineNumbers (const size_t inStartLineNum, const size_t inEndLineNum)
											{mStartLineNum = inStartLineNum; mEndLineNum = inEndLineNum; return *this;}
			//	Printing
			std::ostream &				printInfo (std::ostream & oss) const;
			std::ostream &				printSRT (std::ostream & oss) const;

		//	Instance Data
		private:
			std::string		mRawCaption;	//	Raw caption data (includes all tags)
			SRTSeqNum		mSequenceNum;	//	Sequence number (normally 1-based)
			SRTTimestamp	mStartTime;		//	Start timestamp
			SRTDuration		mDuration;		//	Duration
			SRTWindow		mWindow;		//	Window, if any
			uint16_t		mAnchorPos;		//	Anchor position (0=none 1=botLt 2=botCtr 3=botRt 4=midLt 5=midCtr 6=midRt 7=topLt 8=topCtr 9=topRt)
			size_t			mStartLineNum;	//	Starting SRT source file line number, if any (0=unavailable, 1=1st, 2=2nd, ...)
			size_t			mEndLineNum;	//	Ending SRT source file line number, if any (0=unavailable, 1=1st, 2=2nd, ...)
			bool			mFirstCaption;	//	Is this the first caption of a caption sequence?
	};	//	SRTCaptionInfo


	/**
		@brief	A collection of timed captions.
	**/
	class SRTCaptions
	{
		//	Class Methods & Types
		public:
			static bool					getTimestamp (const std::string & inStr, SRTTimestamp & outTS, std::ostream & oss = std::cerr);
			static bool					getWindow (const std::string & inStr, SRTWindow & outWndo, std::ostream & oss);
			static std::string			timestampStr (const SRTTimestamp inTS, const bool inIncludeRawValue = false);
			static std::string			durationStr (const SRTDuration inDurationMS, const bool inIncludeRawValue = false);
			static std::string			anchorStr (const uint16_t inAnchor, const bool inIncludeRawValue = false);
			static std::string			anchorTag (const uint16_t inAnchor);
			inline static SRTDuration	defaultMinimumDuration (void)								{return gMinCaptionDuration;}
			inline static void			setDefaultMinimumDuration (const SRTDuration inDuration)	{gMinCaptionDuration = inDuration;}

			typedef std::vector<std::string>			StringList;
			typedef std::map<size_t, std::string>		ErrorMap;		//	Line offset (0-based) to error/warning message
			typedef std::map<SRTTimestamp, std::string>	CaptionMap;		//	Caption strings per start timestamp
			typedef std::map<SRTTimestamp, SRTDuration>	DurationMap;	//	Caption durations per start timestamp
			typedef std::map<SRTSeqNum, SRTTimestamp>	TimestampMap;	//	Caption timestamps per sequence number
			typedef std::map<SRTTimestamp, SRTSeqNum>	SeqNumMap;		//	Sequence numbers per start timestamp
			typedef std::map<SRTTimestamp, SRTWindow>	WindowMap;		//	Caption windows per start timestamp
			typedef std::map<SRTTimestamp, uint32_t>	AnchorMap;		//	Caption anchor positions per start timestamp
			typedef std::map<SRTTimestamp, size_t>		SourceMap;		//	Source line numbers (1-based) per start timestamp

		//	Instance Methods
		public:
			//	Modify
			bool					reloadFrom (const StringList & inSRTData);
			bool					reloadFrom (const std::string & inSRTData);
			bool					appendCaption (const SRTCaptionInfo & inCaptionInfo);
			void					clear (void);

			//	Inquiry
			inline bool				hasErrors (void) const		{return errorCount();}
			inline const ErrorMap &	errors (void) const			{return mErrors;}
			inline size_t			errorCount (void) const		{return errors().size();}
			inline bool				hasWarnings (void) const	{return !warnings().empty();}
			inline const ErrorMap &	warnings (void) const		{return mWarnings;}
			inline size_t			warningCount (void) const	{return warnings().size();}
			inline bool				isEmpty (void) const		{return !captionCount();}
			uint32_t				captionCount (void) const;
			SRTSeqNum				minSeqNum (void) const;
			SRTSeqNum				maxSeqNum (void) const;
			inline size_t			linesParsed (void) const	{return mLinesParsed;}
			inline size_t			totalLines (void) const		{return mLines.size();}
			inline std::string		lineAtOffset (const size_t inOffset) const	{return inOffset < mLines.size() ? mLines.at(inOffset) : "";}
			SRTTimestamp			minStartTime (void) const;
			SRTTimestamp			maxStartTime (void) const;
			inline SRTDuration		totalDuration (void) const			{SRTDuration d(0); return maxStartTime() - minStartTime() + (durationAtStartTime(maxStartTime(),d) ? d : 0);}
			bool					hasCaptionAtSeqNum (const SRTSeqNum inSeqNum) const;
			bool					hasCaptionAtStartTime (const SRTTimestamp inStartTime) const;
			SRTTimestamp			startTimeAtSeqNum (const SRTSeqNum inSeqNum) const;
			bool					captionAtStartTime (const SRTTimestamp inStartTime, SRTCaptionInfo & outInfo) const;
			std::string				captionAtStartTime (const SRTTimestamp inStartTime) const;
			bool					durationAtStartTime (const SRTTimestamp inStartTime, SRTDuration & outDuration) const;
			bool					nextCaption (SRTTimestamp & inOutStartTime, SRTCaptionInfo & outInfo) const;
			bool					getWarning (const size_t inIndex0, size_t & outLineOffset, std::string & outMsg) const;
			bool					getError (size_t & outLineOffset, std::string & outMsg) const;
			std::ostream &			printErrors (std::ostream & oss, const bool inIncludeWarnings = true) const;
			std::ostream &			printSRT (std::ostream & oss) const;
			std::ostream &			printInfo (std::ostream & oss) const;

		//	Instance Data
		private:
			TimestampMap	mTimestamps;	//	Sequence-number-to-timestamp map
			SeqNumMap		mSeqNums;		//	Timestamp-to-seqNum map
			CaptionMap		mCaptions;		//	Timestamp-to-caption map
			DurationMap		mDurations;		//	Timestamp-to-duration map
			WindowMap		mWindows;		//	Timestamp-to-window map
			AnchorMap		mAnchors;		//	Timestamp-to-anchor map
			SourceMap		mStartLines;	//	Timestamp-to-starting-source-line-number map (lineNums are +1 from lineOffsets)
			SourceMap		mEndLines;		//	Timestamp-to-ending-source-line-number map (lineNums are +1 from lineOffsets)
			ErrorMap		mErrors;		//	LineNumber-to-ErrorMsg map
			ErrorMap		mWarnings;		//	LineNumber-to-WarningMsg map
			size_t			mLinesParsed;	//	Number of lines parsed
			StringList		mLines;			//	Lines

		//	Class Data
		private:
			static const StringList		kEmptyList;
			static SRTDuration			gMinCaptionDuration;
	};	//	SRTCaptions

#endif	//	__NTV2_SRT_H_
