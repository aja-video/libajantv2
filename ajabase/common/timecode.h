/* SPDX-License-Identifier: MIT */
/**
	@file		timecode.h
	@brief		Declares the AJATimeCode class.
	@copyright	(C) 2010-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_TIMECODE_H
#define AJA_TIMECODE_H
#include "ajabase/common/timebase.h"

/**
	@brief		Identifies the timecode format used in AJATimeCode::QueryString.

	@details	In SDK 17.5, AJA committed to resolving the High Frame-Rate (HFR) timecode display problem.
				SMPTE ST 12-1 (formerly SMPTE 12M) timecode only allows enough bits to code for up to 30 frames
				per second (with the standard officially supporting 24, 25, 30 drop-frame (DF) and 30 non-drop).
				SMPTE ST 12-3 provides a way to encode rates above 30 fps by reusing DBB, field and color frame bits.
				However, neither standard addresses how timecode is actually presented to a human being, particularly
				for high frame rates, which has led to multiple <i>de facto</i> "standards" from multiple companies.
				What follows below is a description of what is henceforth the AJA standard, which is based on HFR
				presentation in various video tape machines (the binary representation of this is what eventually
				became SMPTE ST 12-3) as well as software like the Autodesk Suite that requires users to know (and
				enter) exact frame values.
				
				This class supports 2 types of display conventions. Each is independent of the other and may be matched as needed.
				-	Frame-count convention
				-	Delimiter convention
				
				Frame-count convention applies only to the frame field of a timecode. There are 2 types:
				
				-	<b>LFR</b> (Standard or Default)
					-	Frame values in the frame field are restricted by convention to be less than 30. This supports timecode used in legacy 
						video standards and common in legacy VTRs and hardware.
						-	Frame number is unaltered when frame rate is less than or equal to 30 fps.
						-	Frame number is divided by 2 when frame rate is greater than 30 fps. Fractional frame values are truncated.
						-	Frame number is divided by 4 when frame rate is greater than 60 fps. Fractional frame values are truncated.
					-	Example: Given a 60 fps non-drop frame rate, the frame roll for frames 0, 1, 2, 3, 4, ... 59, 60 are:
						LFR AJA_TIMECODE_LEGACY:	00:00:00:00   00:00:00:00   00:00:00:01   00:00:00:01   00:00:00:02  ...  00:00:00:29   00:00:01:00
						LFR AJA_TIMECODE_STANDARD:	00:00:00:00   00:00:00.00   00:00:00:01   00:00:00.01   00:00:00:02  ...  00:00:00.29   00:00:01:00
				
				-	<b>HFR</b>
					-	This uses the unaltered frame number in the frame field for both low and high frame rates. 
					-	Example: Given a 60 fps non-drop frame rate, the frame roll for frames 0, 1, 2, 3, 4, ... 59, 60 are:
						HFR AJA_TIMECODE_LEGACY:	00:00:00:00   00:00:00:01   00:00:00:02   00:00:00:03   00:00:00:04  ...  00:00:00:59   00:00:01:00
						HFR AJA_TIMECODE_STANDARD:	00:00:00#00   00:00:00#01   00:00:00#02   00:00:00#03   00:00:00#04  ...  00:00:00#59   00:00:01#00
				
				Delimiter conventions applies to the delimiter character-sets which separate fields in the timecode presentation. There are 2 types:
					
					-	::AJA_TIMECODE_LEGACY
						-	This is the default timecode delimiter convention and is widely used in legacy applications. It is not preferred because    
							its presentation is ambiguous when converting between frame values and timecode for high frame rate video.
						-	Non-Drop frame video uses a colon (<tt>":"</tt>) as a delimiter between all fields hours/minutes/seconds/frames.
							Example: <tt>"01:02:03:04"<tt>
						-	Drop frame video also uses a colon (<tt>":"</tt>) as a delimiter between fields hours/minutes/seconds except between 
							seconds/frames field which uses a semi-colon (<tt>";"</tt>)
							Example: <tt>"01:02:03;04"<tt>
						-	This delimiter convention is the same for both LFR and HFR frame-count conventions which can lead to misinterpretation.
					
					-	::AJA_TIMECODE_STANDARD
						-	AJA Standard presentation convention is updated and recommended for current and ongoing adoption.
							- 	Timecode presentation is unambiguous when converting between frame values and timecode string for high frame rate video.
							-	Does not break existing low frame rate (LFR) timecode use cases.
							-	LFR and HFR frame-count presentations are distinct and unambiguous.
						-	In both LFR and HFR presentations, the delimiter between <b>hours/minute/second delimiters:</b> are the same.
							-	Colon (<tt>":"</tt>) for non-drop timecode;
							-	Semi-colon (<tt>";"</tt>) for drop-frame timecode.
						-	LFR frame-count
							-	<b>seconds/frame delimiter:</b>
								-	identical to the other delimiters if the field flag is 0 (or unknown).
								-	a period (<tt>"."</tt>) if the field flag is 1
							-	Roll Sequence Examples:
								-	Drop Frame:		<tt>01;02;03;00   01;02;03.00   01;02;03;01</tt>
								-	Non-Drop Frame: <tt>01:02:03:00   01:02:03.00   01:02:03:01</tt>
						-	HFR frame-count
							- 	<b>seconds/frame delimiter is always a pound sign (<tt>"#"</tt>)</b>
							-	Roll Sequence Examples:
								-	Drop Frame:		<tt>01;02;03#58   01;02;03#59   01;02;04#00</tt>
								-	Non-Drop Frame: <tt>01:02:03#58   01:02:03#59   01:02:04#00</tt>
							-	For frame rates greater than 60, three padded zeros (<tt>"000"</tt>) is used for the frame count
								instead of two (<tt>"00"</tt>).
								-	Roll Sequence Examples for 120 fps drop frame: <tt>01;02;03#099   01;02;03#100   01;02;04#101</tt>

				Notes:
					-	Code that looks for a colon (<tt>":"</tt>) or semicolon (<tt>";"</tt>) in the seconds/frame delimiter
						position to detect drop/non-drop will need to change since it does not work with <b>AJA_TIMECODE_STANDARD</b>.
						It is recommend to use <b>QueryIsDropFrame</b> method since it supports all delimiter conventions presented here.
					-	For HFR, follow SMPTE ST 12-3 interpretation of the binary data; otherwise follow SMPTE ST 12-1 for LFR.
**/
typedef enum
{
	AJA_TIMECODE_LEGACY		= 0,	///< @brief	Legacy notation (e.g. <tt>01:02:03;29 .. 01:02:03:58</tt>)
	AJA_TIMECODE_STANDARD	= 1		///< @brief	AJA standard notation (e.g. <tt>01;02;03.29 .. 01;02;03;29 .. 01:02:03#58</tt>)
} AJATimecodeNotation;


/** \class AJATimeCode timecode.h
 *	\brief Utility class for timecodes.
 *
 *	 This is a storage and conversion class for timecodes.
 */
class AJA_EXPORT AJATimeCode
{
public:
	AJATimeCode();
	AJATimeCode(uint32_t frame, bool bStdTcForHfr=true);
	AJATimeCode(const std::string &str, const AJATimeBase& timeBase, bool bDropFrame, bool bStdTcForHfr=true);
	AJATimeCode(const std::string &str, const AJATimeBase& timeBase);
	AJATimeCode(const AJATimeCode& other);

	virtual ~AJATimeCode();

	/**
	 *	Calculate frame count given hmsf presentation.
	 *
	 *	@param[in]  h				specifies the hours value.
	 *	@param[in]  m				specifies the minutes value.
	 *	@param[in]  s				specifies the seconds value.
	 *	@param[in]  f				specifies the frames value.
	 *	@param[in]	timeBase		frame rate from which to calculate string.
	 *	@param[in]  bDropFrame		true if using drop frame calculation.
	 *	@param[in]	bStdTcForHfr	true to use standard (LFR) frame values (1/2 frame values for framerates above 30 fps, 1/4 for rates above 60)
	 *	@param[in]	addFrame		number of frames to add when bStdTcForHfr=true (e.g given 60 fps, h=m=s=0 f=29 addFrame=1 -> return=59).
	 *	@return Return calculated frame count based on input values
	 */
	static uint32_t CalcFrame(uint32_t h, uint32_t m, uint32_t s, uint32_t f, const AJATimeBase& timeBase, bool bDropFrame, bool bStdTcForHfr, uint32_t addFrame);

	/**
	 *	Calculate hmsf values given frame count. Return add-frame remainder when using std timecode presentation
	 *
	 *	@param[out]  h				specifies the hours value.
	 *	@param[out]  m				specifies the minutes value.
	 *	@param[out]  s				specifies the seconds value.
	 *	@param[out]  f				specifies the frames value.
	 *	@param[in]   frame			frame count.
	 *	@param[in]   timeBase		frame rate from which to calculate hmsf values.
	 *	@param[in]   bDropFrame		true to use drop frame calculation.
	 *	@param[in]	 bStdTcForHfr	true to use standard (LFR) frame values (1/2 frame values for framerates above 30 fps, 1/4 for rates above 60 fps)
	 *	@return Return remaining uncounted frame with bStdTcForHfr=true (e.g given 60 fps, frame=59 => h=m=s=0 f=29 return=1)
	 */
	static uint32_t CalcHmsf(uint32_t &h, uint32_t &m, uint32_t &s, uint32_t &f, uint32_t frame, const AJATimeBase& timeBase, bool bDropFrame, bool bStdTcForHfr);


	/**
	 *	Query string showing timecode for current frame count given the passed parameters.
	 *
	 *	@param[out] str				string in which to place timecode.
	 *	@param[in]	timeBase		frame rate from which to calculate string.
	 *	@param[in]	bDropFrame		drop frame value for string.
	 *	@param[in]	bStdTcForHfr	true to use standard (LFR) frame values (1/2 frame values for framerates above 30 fps, 1/4 for rates above 60)
	 *	@param[in]  notation		sets target output notation for TC delimiters
	 */
	void				QueryString(std::string &str, const AJATimeBase& timeBase, bool bDropFrame, bool bStdTcForHfr, AJATimecodeNotation notation = AJA_TIMECODE_LEGACY);

	/**
	 *	Query string showing timecode for current frame count given the passed parameters.
	 *
	 *	@param[out] str				string in which to place timecode.
	 *	@param[in]	timeBase		frame rate from which to calculate string.
	 *	@param[in]	bDropFrame		drop frame value for string.
	 *	@param[in]  notation		sets target output notation for TC delimiters
	 */
	void				QueryString(std::string &str, const AJATimeBase& timeBase, bool bDropFrame, AJATimecodeNotation notation = AJA_TIMECODE_LEGACY);

	/**
	 *	Query SMPTE string showing timecode for current frame count given the passed parameters.
	 *
	 *	@param[out] pString			buffer in which to place string.
	 *	@param[in]	timeBase		frame rate from which to calculate string.
	 *	@param[in]	bDropFrame		drop frame value for string.
	 */
	void				QuerySMPTEString(char *pString, const AJATimeBase& timeBase, bool bDropFrame);

	/**
	 *	Query SMPTE string byte count.
	 *
	 *	@return		number of bytes in SMPTE timecode string.
	 */
	static int			QuerySMPTEStringSize(void);

	/**
	 * Query frame number.
	 *
	 *	@return		frame number.
	 */
	uint32_t			QueryFrame(void) const;


	/**
	 *	Query HFR divide-by-two flag.
	 *
	 *	@return bStdTc	 Return true when using standard TC notation for HFR (e.g 01:00:00#59 -> 01:00:00:29), set to true by default
	 */
	bool				QueryStdTimecodeForHfr() { return m_stdTcForHfr; }

	/**
	 *	Query hmsf values showing timecode for current frame count given the passed parameters.
	 *
	 *	@param[out] h				specifies the hours value.
	 *	@param[out] m				specifies the minutes value.
	 *	@param[out] s				specifies the seconds value.
	 *	@param[out] f				specifies the frames value.
	 *	@param[in]	timeBase		frame rate from which to calculate string.
	 *	@param[in]	bDropFrame		drop frame value for string.
	 */
	void				QueryHmsf(uint32_t &h, uint32_t &m, uint32_t &s, uint32_t &f, const AJATimeBase& timeBase, bool bDropFrame) const;

	/**
	 *	Set current frame number.
	 *
	 *	@param[in]	frame						new frame number.
	 */
	void				Set(uint32_t frame);

	/**
	 *	Set current frame number.
	 *
	 *	@param[in]	str				xx:xx:xx:xx style string representing new frame number.
	 *	@param[in]	timeBase		frame rate associated with pString.
	 */
	void				Set(const std::string &str, const AJATimeBase& timeBase);

	/**
	 *	Set current frame number.  A variant which may have junk in the string.
	 *
	 *	@param[in]	str				xx:xx:xx:xx style string representing new frame number.
	 *	@param[in]	timeBase		frame rate associated with pString.
	 *	@param[in]	bDrop			true if drop frame
	 */
	void				SetWithCleanup(const std::string &str, const AJATimeBase& timeBase, bool bDrop);

	/**
	 *	Set current frame number.
	 *
	 *	@param[in]	str				xx:xx:xx:xx style string representing new frame number.
	 *	@param[in]	timeBase		frame rate associated with pString.
	 *	@param[in]	bDropFrame		true if forcing dropframe, false otherwise.
	 */
	void				Set(const std::string &str, const AJATimeBase& timeBase, bool bDropFrame);

	/**
	 *	Set current frame number.
	 *
	 *	@param[in]	h				specifies the hours value.
	 *	@param[in]	m				specifies the minutes value.
	 *	@param[in]	s				specifies the seconds value.
	 *	@param[in]	f				specifies the frames value.
	 *	@param[in]	timeBase		frame rate associated with hmsf.
	 *	@param[in]	bDropFrame		true if forcing dropframe, false otherwise.
	 *	@param[in]	bStdTcForHfr	true to use standard (LFR) frame values (1/2 frame values for framerates above 30 fps, 1/4 for rates above 60)
	 *	@param[in]	addFrame		number of frames to add when using std TC presentation (e.g given 60 fps, 00:00:00.29 addFrame=0 => set:frame=58 .. 00:00:00:29 addFrame=1 => set:frame=59).
	 */
	void				SetHmsf(uint32_t h, uint32_t m, uint32_t s, uint32_t f, const AJATimeBase& timeBase, bool bDropFrame, bool bStdTcForHfr, uint32_t addFrame);
	void				SetHmsf(uint32_t h, uint32_t m, uint32_t s, uint32_t f, const AJATimeBase& timeBase, bool bDropFrame);

	/**
	 *	Set timecode via a SMPTE string.
	 *
	 *	@param[in]	pBufr			pointer to string.
	 *	@param[in]	timeBase		time base associated with string.
	 */
	void				SetSMPTEString(const char *pBufr, const AJATimeBase& timeBase);

	/**
	 *	Set timecode via RP188 bytes.
	 *
	 *	@param[in]	inDBB		Specifies the DBB bits of the RP188 struct.
	 *	@param[in]	inLo		Specifies the lo-order 32-bit word of the RP188 struct.
	 *	@param[in]	inHi		Specifies the hi-order 32-bit word of the RP188 struct.
	 *	@param[in]	inTimeBase	Specifies the time base to use.
	 */
	void				SetRP188 (const uint32_t inDBB, const uint32_t inLo, const uint32_t inHi, const AJATimeBase & inTimeBase);

	/**
	 *	Get RP188 register values using the given timebase, and drop frame.
	 *
	 *	@param[out] outDBB		Receives the DBB component.
	 *	@param[out] outLo		Receives the low byte component.
	 *	@param[out] outHi		Receives the high byte component.
	 *	@param[in]	timeBase	Specifies the AJATimeBase to use.
	 *	@param[in]	bDrop		Specify true if forcing drop-frame;	 otherwise false.
	 *	@bug		Unimplemented.
	 *	@todo		Needs to be implemented.
	 */
	void				QueryRP188(uint32_t & outDBB, uint32_t & outLo, uint32_t & outHi, const AJATimeBase & timeBase, const bool bDrop);

	/**
	 *	Set HFR divide-by-two flag.
	 *
	 *	@param[in]	bStdTc	  Set true when using standard TC notation for HFR (e.g 01:00:00#59 -> 01:00:00:29), set to true by default
	 */
	void				SetStdTimecodeForHfr(bool bStdTc) {m_stdTcForHfr = bStdTc;}


	/**
	 *	Query string showing timecode for current frame count given the passed parameters.
	 *
	 *	@param[in]	str						string with timecode
	 */
	static bool			QueryIsDropFrame(const std::string &str);


	static int			QueryStringSize(void);	///< @deprecated	Not needed when using std::string.

	/**
	 *	Query if rp188 data is drop frame or not
	 *
	 *	@param[in]	inDBB	Specifies the DBB bits of the RP188 struct.
	 *	@param[in]	inLo	Specifies the lo-order 32-bit word of the RP188 struct.
	 *	@param[in]	inHi	Specifies the hi-order 32-bit word of the RP188 struct.
	 */
	static bool			QueryIsRP188DropFrame (const uint32_t inDBB, const uint32_t inLo, const uint32_t inHi);

	AJATimeCode&		operator=(const AJATimeCode	 &val);
	AJATimeCode&		operator+=(const AJATimeCode &val);
	AJATimeCode&		operator-=(const AJATimeCode &val);
	AJATimeCode&		operator+=(const int32_t val);
	AJATimeCode&		operator-=(const int32_t val);
	const AJATimeCode	operator+(const AJATimeCode &val) const;
	const AJATimeCode	operator+(const int32_t val) const;
	const AJATimeCode	operator-(const AJATimeCode &val) const;
	const AJATimeCode	operator-(const int32_t val) const;
	bool				operator==(const AJATimeCode &val) const;
	bool				operator<(const AJATimeCode &val) const;
	bool				operator<(const int32_t val) const;
	bool				operator>(const AJATimeCode &val) const;
	bool				operator>(const int32_t val) const;
	bool				operator!=(const AJATimeCode &val) const;

	#if !defined(NTV2_DEPRECATE_17_5)
		void QueryString(char *pStr, const AJATimeBase& tb, bool df, AJATimecodeNotation notation = AJA_TIMECODE_LEGACY);	///< @deprecated	Use QueryString(std::string&, const AJATimeBase&, bool, AJATimecodeNotation) instead
		void QueryRP188(uint32_t *pDbb, uint32_t *pLo, uint32_t *pHi, const AJATimeBase& tb, bool drop);	///< @deprecated	Use QueryRP188(uint32_t&, uint32_t&, uint32_t&, const AJATimeBase&, const bool) instead
	#endif	//	NTV2_DEPRECATE_17_5
private:
	uint32_t			m_frame;
	bool				m_stdTcForHfr;
};	//	AJATimeCode

#endif	// AJA_TIMECODE_H
