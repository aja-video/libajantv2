/**
	@file		ntv2rp188.h
	@brief		Declares the CRP188 class. See SMPTE RP188 standard for details.
	@copyright	(C) 2007-2018 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef __NTV2_RP188_
#define __NTV2_RP188_

#include <string>
#include <sstream>

#include "ajaexport.h"
#include "ajatypes.h"
#include "ntv2enums.h"
#include "videodefines.h"
#include "ntv2publicinterface.h"



#if defined(AJALinux)
# include <stdlib.h>		/* malloc/free */
# include <string.h>		/* memset, memcpy */
#endif

/**
	@brief	Identifies a specific timecode format based on frame rate and whether it's drop-frame or not.
**/
typedef enum
{
	kTCFormatUnknown,
	kTCFormat24fps,
	kTCFormat25fps,
	kTCFormat30fps,
	kTCFormat30fpsDF,
	kTCFormat48fps,
	kTCFormat50fps,
	kTCFormat60fps,
	kTCFormat60fpsDF,
	kTCFormatNumFormats,
	kTCFormatInvalid = kTCFormatNumFormats
} TimecodeFormat;

typedef enum
{
	kTCBurnTimecode,		// display current timecode
	kTCBurnUserBits,		// display current user bits
	kTCBurnFrameCount,		// display current frame count
	kTCBurnBlank			// display --:--:--:--
} TimecodeBurnMode;


const int64_t kDefaultFrameCount = 0x80000000;


/**
	@brief	I'm used for evaluating, manipulating and converting RP188 timecodes.
**/
class AJAExport CRP188
{
public:
	/**
		@name	Construction & Destruction
	**/
	///@{
						CRP188 ();	///< @brief	My default constructor.

						/**
							@brief		Constructs me from another CRP188 instance (copy constructor).
							@param[in]	inRP188		Specifies the ::CRP188 instance to copy.
							@note		This is necessary until the raster rendering is removed from this class.
						**/
						CRP188 (const CRP188 & inRP188);

						/**
							@brief		Constructs me from an ::RP188_STRUCT.
							@param[in]	inRP188		Specifies the ::RP188_STRUCT.
							@param[in]	tcFormat	Optionally specifies the ::TimecodeFormat. Defaults to 30fps.
						**/
						CRP188 (const RP188_STRUCT & inRP188, const TimecodeFormat tcFormat = kTCFormat30fps);

						/**
							@brief		Constructs me from an ::NTV2_RP188.
							@param[in]	inRP188		Specifies the ::NTV2_RP188.
							@param[in]	tcFormat	Optionally specifies the ::TimecodeFormat. Defaults to 30fps.
						**/
						CRP188 (const NTV2_RP188 & inRP188, const TimecodeFormat tcFormat = kTCFormat30fps);

						/**
							@brief		Constructs me from a string that contains a timecode.
							@param[in]	sTimecode	Specifies a string containing a timecode.
							@param[in]	tcFormat	Optionally specifies the ::TimecodeFormat. Defaults to 30fps.
						**/
						CRP188 (const std::string & sTimecode, const TimecodeFormat tcFormat = kTCFormat30fps);

						/**
							@brief		Constructs me from the given timecode component values.
							@param[in]	inFrames	Specifies the "frames" component.
							@param[in]	inSecs		Specifies the "seconds" component.
							@param[in]	inMins		Specifies the "minutes" component.
							@param[in]	inHours		Specifies the "hours" component.
							@param[in]	tcFormat	Optionally specifies the ::TimecodeFormat. Defaults to 30fps.
						**/
						CRP188 (const ULWord inFrames, const ULWord inSecs, const ULWord inMins, const ULWord inHrs, const TimecodeFormat tcFormat = kTCFormat30fps);

						/**
							@brief		Constructs me from a frame count and ::TimecodeFormat.
							@param[in]	inFrameCount	Specifies the frame count.
							@param[in]	tcFormat		Optionally specifies the ::TimecodeFormat. Defaults to 30fps.
						**/
						CRP188 (const ULWord inFrameCount, const TimecodeFormat tcFormat = kTCFormat30fps);

	virtual 			~CRP188();	///< @brief	My destructor.
	///@}

	/**
		@name	Modifiers
	**/
	///@{
	/**
		@brief		Assigns me from another CRP188 instance.
		@param[in]	inRHS	The CRP188 instance being assigned to me.
		@returns	A non-constant reference to me.
		@note		This is necessary until the raster rendering is removed from this class.
	**/
	CRP188 &			operator = (const CRP188 & inRHS);

	/**
		@brief		Resets me using the given timecode component values.
		@param[in]	inFrames	Specifies the "frames" component.
		@param[in]	inSecs		Specifies the "seconds" component.
		@param[in]	inMins		Specifies the "minutes" component.
		@param[in]	inHours		Specifies the "hours" component.
		@param[in]	inFrameRate	Specifies the ::NTV2FrameRate.
		@param[in]	bDropFrame	Specify if this is "drop frame" or not. Defaults to false.
		@param[in]	bSMPTE372	Specify true for SMPTE 372; otherwise false. Defaults to false.
	**/
	void				SetRP188 (const ULWord inFrames, const ULWord inSecs, const ULWord inMins, const ULWord inHours,
									const NTV2FrameRate inFrameRate, const bool bDropFrame = false, const bool bSMPTE372 = false);
	/**
		@brief		Resets me from an ::RP188_STRUCT.
		@param[in]	inRP188		Specifies the ::RP188_STRUCT.
		@param[in]	tcFormat	Optionally changes my ::TimecodeFormat. Defaults to kTCFormatUnknown, which retains my existing format.
	**/
    void				SetRP188 (const RP188_STRUCT & inRP188, const TimecodeFormat tcFormat = kTCFormatUnknown);

	/**
		@brief		Resets me from an ::NTV2_RP188 object.
		@param[in]	inRP188		Specifies the ::NTV2_RP188.
		@param[in]	tcFormat	Optionally changes my ::TimecodeFormat. Defaults to kTCFormatUnknown, which retains my existing format.
	**/
    void				SetRP188 (const NTV2_RP188 & inRP188, const TimecodeFormat tcFormat = kTCFormatUnknown);

	/**
		@brief		Resets me from a string that contains a timecode.
		@param[in]	sTimecode	Specifies a string containing a timecode.
		@param[in]	tcFormat	Optionally changes my ::TimecodeFormat. Defaults to kTCFormatUnknown, which retains my existing format.
	**/
    void				SetRP188 (const std::string & sTimecode, const TimecodeFormat tcFormat = kTCFormatUnknown);

	/**
		@brief		Resets me from the given timecode components.
		@param[in]	inFrames	Specifies the "frames" component.
		@param[in]	inSecs		Specifies the "seconds" component.
		@param[in]	inMins		Specifies the "minutes" component.
		@param[in]	inHours		Specifies the "hours" component.
		@param[in]	inFrameRate	Specifies the ::NTV2FrameRate.
		@param[in]	tcFormat	Optionally changes my ::TimecodeFormat. Defaults to kTCFormatUnknown, which retains my existing format.
	**/
    void				SetRP188 (const ULWord inFrames, const ULWord inSecs, const ULWord inMins, const ULWord inHours, const TimecodeFormat tcFormat = kTCFormatUnknown);

	/**
		@brief		Resets me from the given frame count.
		@param[in]	inFrameCount	Specifies the frame count.
		@param[in]	tcFormat		Optionally changes my ::TimecodeFormat. Defaults to kTCFormatUnknown, which retains my existing format.
	**/
	void				SetRP188 (const ULWord inFrameCount, const TimecodeFormat tcFormat = kTCFormatUnknown);

	/**
		@brief		Sets or clears my drop-frame flag (including in my ::RP188_STRUCT member).
		@param[in]	bDropFrameFlag	Specify true to set my drop-frame flag;  otherwise false to clear it.
	**/
	void				SetDropFrame (const bool bDropFrameFlag);

	/**
		@brief		Sets or clears my color-frame flag (including in my ::RP188_STRUCT member).
		@param[in]	bColorFrameFlag	Specify true to set my color-frame flag;  otherwise false to clear it.
	**/
	void				SetColorFrame (const bool bColorFrameFlag);

	void				SetVaricamFrameActive (bool bVaricamActive, ULWord frame);
	void				SetVaricamRate (NTV2FrameRate frameRate);

	/**
		@brief		Sets my field ID (including in my ::RP188_STRUCT member).
		@param[in]	inFieldID	The new field ID value to use. Please specify only 0 or 1.
	**/
	void				SetFieldID (const ULWord inFieldID);

	void				SetBFGBits (bool bBFG0, bool bBFG1, bool bBFG2);
	void				SetSource (UByte src);
	void				SetOutputFilter (UByte src);

	/**
		@brief		Increments my frame count. Accounts for 24-hour wrap in my ::TimecodeFormat.
		@param[in]	inNumFrames		The number of frames to increment.
		@returns	My new frame count.
	**/
	ULWord				AddFrames (const ULWord inNumFrames);

	/**
		@brief		Decrements my frame count by a given amount. Accounts for 24-hour "underflow" in my ::TimecodeFormat.
		@param[in]	inNumFrames		The number of frames to decrement. (It's okay to exceed MaxFramesPerDay.)
		@returns	My new frame count.
	**/
	ULWord				SubtractFrames (const ULWord inNumFrames);

	/**
		@brief		Increments my frame count by a given amount.
		@param[in]	inNumFrames		The number of frames to increment.
		@returns	A non-constant reference to me.
		@see		CRP188::AddFrames
	**/
	inline CRP188 &		operator += (const unsigned inRHS)			{ AddFrames(ULWord(inRHS));  return *this; }

	/**
		@brief		Decrements my frame count by a given amount.
		@param[in]	inNumFrames		The number of frames to decrement.
		@returns	A non-constant reference to me.
		@see		CRP188::SubtractFrames
	**/
	inline CRP188 &		operator -= (const unsigned inRHS)			{ SubtractFrames(ULWord(inRHS));  return *this; }
	///@}

	/**
		@name	Inquiry
	**/
	///@{
	/**
		@returns	True if I've been properly initialized;  otherwise false.
	**/
	inline bool					IsInitialized (void) const						{ return _bInitialized; }

	/**
		@brief		Answers with my current cached timecode string.
		@param[out]	outValue	Receives my current cached timecode string value.
		@returns	True if I've been properly initialized;  otherwise false.
	**/
    inline bool					GetRP188Str  (std::string & outStr) const		{ outStr = GetRP188Str();  return _bInitialized; }

	/**
		@returns	A constant reference to my cached timecode string.
		@note		The string contents won't be valid if I haven't been properly initialized.
	**/
    inline const std::string &	GetRP188Str  (void) const						{ return _sHMSF; }

	/**
		@brief		Answers with my current cached "frames" value.
		@param[out]	outValue	Receives my current cached "frames" value.
		@returns	True if I've been properly initialized;  otherwise false.
	**/
	inline bool					GetRP188Frms (ULWord & outValue) const			{ outValue = _ulVal[0];  return IsInitialized(); }

	/**
		@brief		Answers with my current cached "seconds" value.
		@param[out]	outValue	Receives my current cached "seconds" value.
		@returns	True if I've been properly initialized;  otherwise false.
	**/
	inline bool					GetRP188Secs (ULWord & outValue) const			{ outValue = _ulVal[1];  return IsInitialized(); }

	/**
		@brief		Answers with my current cached "minutes" value.
		@param[out]	outValue	Receives my current cached "minutes" value.
		@returns	True if I've been properly initialized;  otherwise false.
	**/
	inline bool					GetRP188Mins (ULWord & outValue) const			{ outValue = _ulVal[2];  return IsInitialized(); }

	/**
		@brief		Answers with my current cached "hours" value.
		@param[out]	outValue	Receives my current cached "hours" value.
		@returns	True if I've been properly initialized;  otherwise false.
	**/
	inline bool					GetRP188Hrs  (ULWord & outValue) const			{ outValue = _ulVal[3];  return IsInitialized(); }

	/**
		@brief		Answers with the current value of my ::RP188_STRUCT.
		@param[out]	outRP188	Receives my current RP188 "registers" value.
		@returns	True if I've been properly initialized;  otherwise false.
	**/
	inline bool					GetRP188Reg  (RP188_STRUCT & outRP188) const	{ outRP188 = _rp188;	return IsInitialized(); }

	/**
		@returns	A constant reference to my RP188 "registers" value.
	**/
	inline const RP188_STRUCT &	GetRP188Reg  (void) const						{ return _rp188; }

	/**
		@brief		Answers with the current value of my RP188 "registers" value.
		@param[out]	outRP188	Receives my current RP188 "registers" value as an ::NTV2_RP188 object.
		@returns	True if I've been properly initialized;  otherwise false.
	**/
	inline bool					GetRP188Reg  (NTV2_RP188 & outRP188) const		{ outRP188 = _rp188;	return IsInitialized(); }

	/**
		@returns	My current RP188 "registers" value as an ::NTV2_RP188 object.
	**/
	inline NTV2_RP188			GetNTV2RP188 (void) const						{ return NTV2_RP188(_rp188); }

	/**
		@returns	True if my current Field ID bit is set;  otherwise false if it's clear.
		@note		The returned value won't be valid if I haven't been properly initialized.
	**/
	bool						GetFieldID (void);

	/**
		@brief		Answers with my total frame count based on my ::TimecodeFormat and my cached
					hours, minutes, seconds and frames components.
		@param[out]	outFrameCount	Receives the calculated total frame count.
		@returns	True if I've been properly initialized;  otherwise false.
	**/
	bool						GetFrameCount (ULWord & outFrameCount) const;

	/**
		@returns	The maximum number of frames that are possible for a 24-hour period.
		@param[in]	tcFormat	Optionally specifies a different ::TimecodeFormat than my own.
								Defaults to kTCFormatUnknown, which uses my current ::TimecodeFormat.
	**/
	ULWord						MaxFramesPerDay (const TimecodeFormat tcFormat = kTCFormatUnknown) const;

	inline const std::string &	GetRP188UserBitsStr (void)			{ RP188ToUserBits();  return _sUserBits; }
	bool						GetRP188UserBitsStr (std::string & sRP188UB);

	/**
		@returns	The source component byte of my cached DBB field.
	**/
	inline UByte				GetSource (void) const				{ return UByte((_rp188.DBB & 0xFF000000) >> 24); }

	/**
		@returns	The output filter component byte of my cached DBB field.
	**/
	inline UByte				GetOutputFilter (void) const		{ return UByte(_rp188.DBB & 0x000000FF); }

	/**
		@returns	My current ::TimecodeFormat.
	**/
	inline TimecodeFormat		GetTimecodeFormat (void) const		{ return _tcFormat; }

	/**
		@brief		Dumps me in a human-readable format to the given output stream.
		@param		theStream	The output stream in which to stream the dump.
		@param[in]	inCompact	Optionally specifies if the Dump should be in a compact form.
								Defaults to true.
		@returns	The given output stream.
	**/
	std::ostream &				Dump (std::ostream & theStream, const bool inCompact = true) const;

	inline NTV2_SHOULD_BE_DEPRECATED(const char *	GetRP188CString (void) const)		{ return GetRP188Str().c_str(); }
	inline NTV2_SHOULD_BE_DEPRECATED(const char *	GetRP188UserBitsCString (void))		{ return GetRP188UserBitsStr().c_str(); }
	///@}


	/**
		@name	Conversion
	**/
	///@{
	/**
		@brief		Converts the given timecode components into a frame count.
		@param[out]	outFrameCount	Receives the calculated total frame count.
		@param[in]	tcFormat		Specifies the ::TimecodeFormat to use.
									Specify kTCFormatUnknown to use my ::TimecodeFormat.
		@param[in]	inHours			Specifies the "hours" component to use in the calculation.
		@param[in]	inMins			Specifies the "minutes" component to use in the calculation.
		@param[in]	inSecs			Specifies the "seconds" component to use in the calculation.
		@param[in]	inFrames		Specifies the "frames" component to use in the calculation.
	**/
	void						ConvertTimecode (ULWord & outFrameCount, const TimecodeFormat tcFormat, const ULWord inHours, const ULWord inMins, const ULWord inSecs, const ULWord inFrames) const;

	void						ConvertFrameCount (ULWord frameCount, TimecodeFormat format, ULWord & hours, ULWord & minutes, ULWord & seconds, ULWord & frames);
	///@}


	/**
		@name	Attribute Testing
	**/
	///@{
	/**
		@returns	True if my RP188 data has been marked "fresh" this past frame;  otherwise false.
		@note		This result is valid only if I was instantiated or set using an ::RP188_STRUCT.
	**/
	inline bool			IsFreshRP188 (void) const				{ return _bFresh; }

	/**
		@returns	True if my Varicam "Frame 0 Active" bit is set;  otherwise false.
		@note		This result is valid only if I was instantiated or set using an ::RP188_STRUCT.
	**/
	inline bool			VaricamFrame0 (void) const				{ return _bVaricamActiveF0; }

	/**
		@returns	True if my Varicam "Frame 1 Active" bit is set;  otherwise false.
		@note		This result is valid only if I was instantiated or set using an ::RP188_STRUCT.
	**/
	inline bool			VaricamFrame1 (void) const				{ return _bVaricamActiveF1; }
	ULWord				VaricamFrameRate (void);
	bool				FormatIsDropFrame (TimecodeFormat format = kTCFormatUnknown) const;
	bool				FormatIs60_50fps (TimecodeFormat format = kTCFormatUnknown) const;
	bool				FormatIsPAL (TimecodeFormat format = kTCFormatUnknown) const;

	inline ULWord		FieldID (void) const					{ return _fieldID; }			// fieldID bit
	inline bool			DropFrame (void) const					{ return _bDropFrameFlag;  }	// drop frame bit
	inline bool			ColorFrame (void) const					{ return _bColorFrameFlag; }	// color frame bit
	ULWord				BinaryGroup (ULWord smpteNum);
	///@}

	// For historical reasons, calling SetRP188 clears the user bits, so a call to either of these two functions
	// should be done after the timecode value is set
	bool				SetBinaryGroup (ULWord smpteNum, ULWord bits);
	bool				SetUserBits (ULWord bits);					// eight groups of four bits each

	ULWord				UDW (ULWord smpteUDW);

	/**
		@return	The frame rate of the given TimecodeFormat.
		@note	This function doesn't deal (accurately) with drop-frame.
	**/
	ULWord				FramesPerSecond (TimecodeFormat format = kTCFormatUnknown) const;
	NTV2FrameRate		DefaultFrameRateForTimecodeFormat (TimecodeFormat format = kTCFormatUnknown) const;

	/**
		@name	Comparing
	**/
	///@{
	/**
		@returns	True if my cached hours, minutes, seconds and frames values all match those of the operand's;
					otherwise false.
		@param[in]	inRHS	The right-hand-side operand I'm to be compared with.
	**/
	bool				operator == (const CRP188 & inRHS) const;

	/**
		@returns	True if my total frame count is less than that of the operand's;  otherwise false.
		@param[in]	inRHS	The right-hand-side operand I'm being compared with.
	**/
	bool				operator < (const CRP188 & inRHS) const;	//	Returns true if my frame count < RHS

	/**
		@returns	The difference, in total frames, between me and the operand.
		@param[in]	inRHS	The right-hand-side operand whose total frame count is being subtracted from my own.
	**/
	int					operator - (const CRP188 & inRHS) const;	//	Returns difference, in frames

	/**
		@returns	The absolute difference, in total frames, between me and the operand.
		@param[in]	inRHS	The right-hand-side operand whose total frame count is being subtracted from my own.
	**/
	inline ULWord		FramesApart (const CRP188 & inRHS) const	{return ULWord(abs(*this - inRHS));}

	/**
		@returns	True if the absolute difference, in total frames, between me and the operand is exactly 1;
					otherwise false.
		@param[in]	inTC	The right-hand-side timecode operand.
	**/
	inline bool			IsAdjacent (const CRP188 & inTC) const		{return FramesApart(inTC) == 1;}
	///@}

	/**
		@name	Raster Burn-In
	**/
	///@{
	bool				InitBurnIn (NTV2FrameBufferFormat frameBufferFormat, NTV2FrameDimensions frameDimensions, LWord percentY = 0);
	void				writeV210Pixel (char **pBytePtr, int x, int c, int y);
	bool				BurnTC (char *pBaseVideoAddress, int rowBytes, TimecodeBurnMode burnMode, int64_t frameCount = kDefaultFrameCount, bool bDisplay60_50fpsAs30_25 = false);
	void				CopyDigit (char *pDigit, int digitWidth, int digitHeight, char *pFrameBuff, int fbRowBytes);
	std::string			GetTimeCodeString(bool bDisplay60_50fpsAs30_25 = false);
	///@}

	/**
		@returns	A string containing a readable form of a given ::TimecodeFormat.
		@param[in]	inFormat			The ::TimecodeFormat to convert into a string.
		@param[in]	inIsHumanReadable	If true, use a human-readable (and more compact) format;
										otherwise, use the symbolic constant used in this header file.
										Defaults to false (the longer, technical format).
	**/
	static std::string	TimecodeFormatToString (const TimecodeFormat inFormat, const bool inIsHumanReadable = false);

private:
    void				ConvertTcStrToVal (void);   // converts _sHMSF to _ulVal
    void				ConvertTcStrToReg (void);   // converts _sHMSF to _rp188
	void				RP188ToUserBits (void);		// derives _ulUserBits and _sUserBits from RP188 struct
	void				Init (void);


private:
	TimecodeFormat		_tcFormat;			// fps, drop- or non-drop frame
    bool				_bInitialized;		// if constructed with no args, set to false
	bool				_bFresh;			// true if hardware told us this was new ANC data (not just what was lying around in the registers)
	//bool				_bDropFrame;		// we have to be told whether we are df or ndf
	//bool				_b50Hz;				// if true, interpret FieldID and Binary Group Flags per 50 Hz spec

	// RP188 user bits
	bool				_bVaricamActiveF0;  // Varicam "Active Frame 0" user bit flag
	bool				_bVaricamActiveF1;  // Varicam "Active Frame 1" user bit flag
	ULWord				_fieldID;			// FieldID bit: '0' or '1'
	bool				_bDropFrameFlag;	// Drop Frame bit: '0' or '1'
	bool				_bColorFrameFlag;   // Color Frame bit: '0' or '1'
	ULWord				_varicamRate;		// Varicam rate expressed as bits [0..3] 1 units, bits [4..7] tens unit.

    std::string			_sHMSF;				// hour:minute:second:frame in string format
	std::string			_sUserBits;			// Binary Groups 8-1 in string format
    ULWord				_ulVal[4];			// [0]=frame, [1]=seconds, etc.
	ULWord				_ulUserBits[8];		// [0] = Binary Group 1, [1] = Binary Group 2, etc. (note: SMPTE labels them 1 - 8)
    RP188_STRUCT		_rp188;				// AJA native format

	bool				_bRendered;			// set 'true' when Burn-In character map has been rendered
	char *				_pCharRenderMap;	// ptr to rendered Burn-In character set
	NTV2FrameBufferFormat _charRenderFBF;	// frame buffer format of rendered characters
	ULWord				_charRenderHeight;	// frame height for which rendered characters were rendered
	ULWord				_charRenderWidth;	// frame width for which rendered characters were rendered
	int					_charWidthBytes;	// rendered character width in bytes
	int					_charHeightLines;	// rendered character height in frame lines
	int					_charPositionX;		// offset (in bytes) from left side of screen to first burn-in character
	int					_charPositionY;		// offset (in lines) from top of screen to top of burn-in characters
};	//	CRP188

/**
	@brief		Prints the given CRP188's contents into the given output stream.
	@param		outputStream	The stream into which the human-readable timecode will be written.
	@param[in]	inObj			Specifies the CRP188 instance to be streamed.
	@return		The ostream that was specified.
**/
AJAExport std::ostream & operator << (std::ostream & outputStream, const CRP188 & inObj);

#endif	//	__NTV2_RP188_
