/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2ccplayer.cpp
	@brief		Implementation of NTV2CCPlayer class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2ccplayer.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2debug.h"
#include "ntv2democommon.h"
#include "ntv2rp188.h"
#include "ntv2transcode.h"
#include "ntv2utils.h"
#include "ajabase/common/types.h"
#include "ajabase/common/ajarefptr.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/process.h"
#include "ntv2testpatterngen.h"
#include "ajabase/common/videotypes.h"
#include "ajabase/system/debugshare.h"
#include "ajabase/common/timecode.h"
#include "ajabase/common/common.h"
#include "ajaanc/includes/ancillarylist.h"
#include "ajaanc/includes/ancillarydata_cea608_vanc.h"
#include "ajaanc/includes/ancillarydata_cea708.h"
#include "ntv2captionrenderer.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <deque>


using namespace std;


#define	AsSCCSource(__x__)	reinterpret_cast<SCCSource&>(__x__)
#define AsUBytePtr(__x__)	reinterpret_cast<UByte*>(__x__)
//#define	MEASURE_ACCURACY	1		//	Enables a feedback mechanism to precisely generate captions at the desired rate

static const uint32_t	kAppSignature	(NTV2_FOURCC('C','C','P','L'));


/**
	@brief	Some default text to use if standard input isn't used.
**/
static const string gBuiltInCaptions ("IN CONGRESS, July 4, 1776.\n"
	"The unanimous Declaration of the thirteen united States of America.\n"
	"When in the Course of human events, it becomes necessary for one people to dissolve the political bands which have connected them with another, "
	"and to assume among the powers of the earth, the separate and equal station to which the Laws of Nature and of Nature's God entitle them, a decent "
	"respect to the opinions of mankind requires that they should declare the causes which impel them to the separation.\n"
	"We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, "
	"that among these are Life, Liberty and the pursuit of Happiness. That to secure these rights, Governments are instituted among Men, deriving their "
	"just powers from the consent of the governed, That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, "
	"and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and "
	"Happiness. Prudence, indeed, will dictate that Governments long established should not be changed for light and transient causes; and accordingly all experience hath shewn, "
	"that mankind are more disposed to suffer, while evils are sufferable, than to right themselves by abolishing the forms to which they are accustomed. "
	"But when a long train of abuses and usurpations, pursuing invariably the same Object evinces a design to reduce them under absolute Despotism, it is their right, "
	"it is their duty, to throw off such Government, and to provide new Guards for their future security. Such has been the patient sufferance of these Colonies; and such is now "
	"the necessity which constrains them to alter their former Systems of Government. The history of the present King of Great Britain is a history of repeated injuries and "
	"usurpations, all having in direct object the establishment of an absolute Tyranny over these States. To prove this, let Facts be submitted to a candid world.\n"
	"He has refused his Assent to Laws, the most wholesome and necessary for the public good.\n"
	"He has forbidden his Governors to pass Laws of immediate and pressing importance, unless suspended in their operation till his Assent should be obtained; "
	"and when so suspended, he has utterly neglected to attend to them.\n"
	"He has refused to pass other Laws for the accommodation of large districts of people, unless those people would relinquish the right of Representation in the Legislature, "
	"a right inestimable to them and formidable to tyrants only.\n"
	"He has called together legislative bodies at places unusual, uncomfortable, and distant from the depository of their public Records, for the sole purpose of fatiguing them "
	"into compliance with his measures. \n"
	"He has dissolved Representative Houses repeatedly, for opposing with manly firmness his invasions on the rights of the people.\n"
	"He has refused for a long time, after such dissolutions, to cause others to be elected; whereby the Legislative powers, incapable of Annihilation, have returned to the People "
	"at large for their exercise; the State remaining in the mean time exposed to all the dangers of invasion from without, and convulsions within.\n"
	"He has endeavoured to prevent the population of these States; for that purpose obstructing the Laws for Naturalization of Foreigners; refusing to pass others to encourage "
	"their migrations hither, and raising the conditions of new Appropriations of Lands.\n"
	"He has obstructed the Administration of Justice, by refusing his Assent to Laws for establishing Judiciary powers.\n"
	"He has made Judges dependent on his Will alone, for the tenure of their offices, and the amount and payment of their salaries.\n"
	"He has erected a multitude of New Offices, and sent hither swarms of Officers to harrass our people, and eat out their substance.\n"
	"He has kept among us, in times of peace, Standing Armies without the Consent of our legislatures.\n"
	"He has affected to render the Military independent of and superior to the Civil power.\n"
	"He has combined with others to subject us to a jurisdiction foreign to our constitution, and unacknowledged by our laws; "
	"giving his Assent to their Acts of pretended Legislation:\n"
	"For Quartering large bodies of armed troops among us:\n"
	"For protecting them, by a mock Trial, from punishment for any Murders which they should commit on the Inhabitants of these States:\n"
	"For cutting off our Trade with all parts of the world:\n"
	"For imposing Taxes on us without our Consent:\n"
	"For depriving us in many cases, of the benefits of Trial by Jury:\n"
	"For transporting us beyond Seas to be tried for pretended offences.\n"
	"For abolishing the free System of English Laws in a neighbouring Province, establishing therein an Arbitrary government, and enlarging its Boundaries so as to render it at "
	"once an example and fit instrument for introducing the same absolute rule into these Colonies:\n"
	"For taking away our Charters, abolishing our most valuable Laws, and altering fundamentally the Forms of our Governments:\n"
	"For suspending our own Legislatures, and declaring themselves invested with power to legislate for us in all cases whatsoever.\n"
	"He has abdicated Government here, by declaring us out of his Protection and waging War against us.\n"
	"He has plundered our seas, ravaged our Coasts, burnt our towns, and destroyed the lives of our people.\n"
	"He is at this time transporting large Armies of foreign Mercenaries to compleat the works of death, desolation and tyranny, already begun with circumstances of Cruelty & "
	"perfidy scarcely paralleled in the most barbarous ages, and totally unworthy the Head of a civilized nation.\n"
	"He has constrained our fellow Citizens taken Captive on the high Seas to bear Arms against their Country, to become the executioners of their friends and Brethren, "
	"or to fall themselves by their Hands.\n"
	"He has excited domestic insurrections amongst us, and has endeavoured to bring on the inhabitants of our frontiers, the merciless Indian Savages, whose known rule of warfare, "
	"is an undistinguished destruction of all ages, sexes and conditions.\n"
	"In every stage of these Oppressions We have Petitioned for Redress in the most humble terms: Our repeated Petitions have been answered only by repeated injury. "
	"A Prince whose character is thus marked by every act which may define a Tyrant, is unfit to be the ruler of a free people.\n"
	"Nor have We been wanting in attentions to our Brittish brethren. We have warned them from time to time of attempts by their legislature to extend an unwarrantable "
	"jurisdiction over us. We have reminded them of the circumstances of our emigration and settlement here. We have appealed to their native justice and magnanimity, "
	"and we have conjured them by the ties of our common kindred to disavow these usurpations, which, would inevitably interrupt our connections and correspondence. "
	"They too have been deaf to the voice of justice and of consanguinity. We must, therefore, acquiesce in the necessity, which denounces our Separation, and hold them, "
	"as we hold the rest of mankind, Enemies in War, in Peace Friends.\n"
	"We, therefore, the Representatives of the united States of America, in General Congress, Assembled, appealing to the Supreme Judge of the world for the rectitude of "
	"our intentions, do, in the Name, and by Authority of the good People of these Colonies, solemnly publish and declare, That these United Colonies are, and of Right ought "
	"to be Free and Independent States; that they are Absolved from all Allegiance to the British Crown, and that all political connection between them and the State of Great "
	"Britain, is and ought to be totally dissolved; and that as Free and Independent States, they have full Power to levy War, conclude Peace, contract Alliances, establish "
	"Commerce, and to do all other Acts and Things which Independent States may of right do. And for the support of this Declaration, with a firm reliance on the protection of "
	"divine Providence, we mutually pledge to each other our Lives, our Fortunes and our sacred Honor.");

/**
	@brief	The global built-in caption data input stream.
**/
static istringstream	gBuiltInStream (gBuiltInCaptions);

#if defined (MEASURE_ACCURACY)

	/**
		@brief	I'm a simple object that measures time between successive Sample calls as a moving average.
		@note	I am not thread-safe.
	**/
	class Speedometer
	{
		public:
			/**
				@brief	Constructs me to accommodate a given number of measurement samples.
				@param[in]	inNumSamples	Specifies the maximum number of measurements I will accommodate.
											Defaults to 20.
			**/
			Speedometer (const size_t inNumSamples = 20)
				:	mMaxNumSamples		(inNumSamples),
					mSampleCallTally	(0)
			{
				mTimespanSamples.push_back (10);	//	Initial seed of 10 millisec
				mWhenLastSampleCall = AJATime::GetSystemMilliseconds ();
			}

			/**
				@brief	Enqueues the elapsed time since this method was last called,
						discarding the oldest measurement (if my queue is full).
			**/
			virtual void	Sample (void)
			{
				if (mTimespanSamples.size () >= mMaxNumSamples)
					mTimespanSamples.pop_front ();
				const uint64_t	now	(AJATime::GetSystemMilliseconds ());
				mTimespanSamples.push_back (now - mWhenLastSampleCall);
				mWhenLastSampleCall = now;
				mSampleCallTally++;
			}

			/**
				@return	The average time between Sample calls, in milliseconds.
			**/
			virtual double	GetAvgMilliSecsBetweenSamples (void) const
			{
				uint64_t	sum	(0.0);
				for (SampleQueueConstIter iter (mTimespanSamples.begin ()); iter != mTimespanSamples.end (); ++iter)
					sum += *iter;
				return double (sum) / double (mTimespanSamples.size ());
			}

			/**
				@return	The average number of Sample calls per second.
			**/
			virtual double	GetAvgSamplesPerSecond (void) const
			{
				uint64_t	sum	(0.0);
				for (SampleQueueConstIter iter (mTimespanSamples.begin ()); iter != mTimespanSamples.end (); ++iter)
					sum += *iter;
				return double (mTimespanSamples.size ()) * 1000.0 / double (sum);
			}

			virtual inline double	GetAvgSamplesPerMinute (void) const		{return GetAvgSamplesPerSecond () * 60.0;}	///< @return	The average number of Sample calls per minute.
			virtual inline size_t	GetSampleCount (void) const				{return mTimespanSamples.size ();}			///< @return	The number of measurements currently in my queue.
			virtual inline uint64_t	GetSampleCountTally (void) const		{return mSampleCallTally;}					///< @return	The number of times Sample was called.

		private:
			typedef std::deque <uint64_t>		SampleQueue;
			typedef	SampleQueue::const_iterator	SampleQueueConstIter;

			SampleQueue		mTimespanSamples;		///< @brief	Queue of timespan samples
			const size_t	mMaxNumSamples;			///< @brief	Stipulates my maximum queue size
			uint64_t		mWhenLastSampleCall;	///< @brief	When Sample method was last called
			uint64_t		mSampleCallTally;		///< @brief	Tally of number of times Sample method was called

	};	//	Speedometer


	/**
		@brief	Streams the given Speedometer instance to the given output stream in a human-readable format.
		@param		inOutStr	Specifies the output stream that is to receive the Speedometer in a human-readable format.
		@param[in]	inObj		Specifies the Speedometer instance to be streamed.
		@return	A reference to the given output stream.
	**/
	static ostream & operator << (ostream & inOutStr, const Speedometer & inObj)
	{
		const double	samplesPerSecond	(inObj.GetAvgSamplesPerSecond ());
		const double	samplesPerMinute	(samplesPerSecond * 60.0);
		return inOutStr << samplesPerMinute << " per min";
	}

	static uint64_t gWhenLastDisplay (0);		///	Used to periodically show timing information

#endif	//	MEASURE_ACCURACY

AJALabelValuePairs CCGenConfig::Get (void) const
{
	static const string EndActions[] = {"Quit", "Repeat", "Idle", ""};
	AJALabelValuePairs result;
	string	filesToPlay(aja::join(fFilesToPlay, "\n"));
	if (filesToPlay.empty())	filesToPlay = "<default>";
	AJASystemInfo::append(result, "Files to Play",			filesToPlay);
	AJASystemInfo::append(result, "End Action",				EndActions[fEndAction]);
	AJASystemInfo::append(result, "Caption Mode",			::NTV2Line21ModeToStr(fCaptionMode));
	AJASystemInfo::append(result, "Caption Channel",		::NTV2Line21ChannelToStr(fCaptionChannel));
	AJASystemInfo::append(result, "Newlines Make New Rows",	fNewLinesAreNewRows ? "Y" : "N");
	AJASystemInfo::append(result, "Chars Per Minute",		aja::to_string(fCharsPerMinute));
	AJASystemInfo::append(result, "Attributes",				::NTV2Line21AttributesToStr(fAttributes));
	return result;
}

AJALabelValuePairs CCPlayerConfig::Get (const bool inCompact) const
{
	AJALabelValuePairs result (PlayerConfig::Get (inCompact));
	AJASystemInfo::append(result, "Background Pattern",	fTestPatternName);
	AJASystemInfo::append(result, "Emit Statistics",	fEmitStats ? "Y" : "N");
	AJASystemInfo::append(result, "Force RTP",			fForceRTP ? (fForceRTP&2 ? "MultiPkt" : "UniPkt") : "Normal");
	AJASystemInfo::append(result, "Suppress Audio",		fSuppressAudio ? "Y" : "N");
	AJASystemInfo::append(result, "Suppress Line21",	fSuppressLine21 ? "Y" : "N");
	AJASystemInfo::append(result, "Suppress 608 Pkt",	fSuppress608 ? "Y" : "N");
	AJASystemInfo::append(result, "Suppress 708 Pkt",	fSuppress708 ? "Y" : "N");
	AJASystemInfo::append(result, "Suppress Timecode",	fSuppressTimecode ? "Y" : "N");
	for (CaptionChanGenMapCIter it(fCapChanGenConfigs.begin());  it != fCapChanGenConfigs.end();  ++it)
	{
		AJASystemInfo::append(result, ::NTV2Line21ChannelToStr(it->first, false));
		AJALabelValuePairs pairs(it->second.Get());
		for (AJALabelValuePairsConstIter iter(pairs.begin());  iter != pairs.end();  ++iter)
			result.push_back(*iter);
	}
	return result;
}

std::ostream & operator << (std::ostream & ioStrm,  const CCPlayerConfig & inObj)
{
	return ioStrm << AJASystemInfo::ToString(inObj.Get());
}


//////////////////////////////////////////////
//	Caption Source
//////////////////////////////////////////////

/**
	@brief	I read text from an input stream and I "package" it into "sentences" that are up
			to 32 characters long, suitable for captioning. Clients specify the rate at which
			I generate captions, and the input stream that I read from, then repeatedly call
			my GetNextCaptionRow function to obtain each successive "sentence". Upon reaching
			end-of-file, I enter a "finished" state and thereafter return empty strings.
	@note	The current design doesn't handle Unicode characters.
**/
class CaptionSource;
typedef AJARefPtr <CaptionSource>	CaptionSourcePtr;


/**
	@brief	I am an ordered sequence of CaptionSources.
**/
typedef deque <CaptionSourcePtr>			CaptionSourceList;
typedef CaptionSourceList::const_iterator	CaptionSourceListConstIter;


class CaptionSource
{
	//	Instance Methods
	public:
		/**
			@brief	My only constructor.
			@param[in]	inCharsPerMinute		Specifies the maximum rate at which I will emit characters.
			@param[in]	pInInputStream			Specifies a valid, non-NULL pointer to the input stream to use.
			@param[in]	inFilePath				Specifies the input stream file path (if any)
			@param[in]	inDeleteInputStream		If true, I will delete the input stream object when I'm destroyed;
												otherwise it's the caller's responsibility.
		**/
		explicit CaptionSource (const double inCharsPerMinute, istream * pInInputStream, const string & inFilePath, const bool inDeleteInputStream = false)
			:	mpInputStream		(pInInputStream),
				mInputFilePath		(inFilePath),
				mCharsPerMinute		(inCharsPerMinute),
				mMaxRowCharWidth	(32),
				mFinished			(false),
				mDeleteInputStream	(inDeleteInputStream),
				mIsTextMode			(false),
				mMilliSecsPerChar	(0.0),
				mCaptionChannel		(NTV2_CC608_ChannelInvalid)
		{
			NTV2_ASSERT(mpInputStream);						//	Must be non-NULL pointer
			NTV2_ASSERT(mCharsPerMinute > 0.0);				//	Must be greater than zero
			*mpInputStream >> std::noskipws;				//	Include whitespace when reading from this stream
			mMilliSecsPerChar = 60000.0 / mCharsPerMinute;	//	Milliseconds of delay (sleep time) between emitting successive characters
		}


		/**
			@brief	My destructor. If was told at construction time that I was to be responsible
					for deleting the input stream, then I'll delete it here.
		**/
		virtual ~CaptionSource ()
		{
			if (mDeleteInputStream && mpInputStream)
			{
				delete mpInputStream;
				mpInputStream = AJA_NULL;
			}
			#if defined (MEASURE_ACCURACY)
				PLDBG(::NTV2Line21ChannelToStr(mCaptionChannel) << ": " << mCharSpeed.GetSampleCountTally() << " final chars: " << mCharSpeed);
			#endif	//	MEASURE_ACCURACY
		}

		/**
			@brief		Returns the next character read from my input stream. If my text input stream has reached
						EOF, I set my "finished" flag and return zero.
			@return		A string containing a single UTF8-encoded unicode character.
		**/
		virtual string GetNextCaptionCharacter (void)
		{
			static double	milliSecsPerChar(mMilliSecsPerChar);
			string			resultChar;
			unsigned char	rawBytes[5]	= {0, 0, 0, 0, 0};

			AJATime::Sleep (int32_t(milliSecsPerChar));
			if (mpInputStream->tellg() == 0  &&  !mInputFilePath.empty())
				PLNOTE("Starting caption file '" << mInputFilePath << "'");

			*mpInputStream >> rawBytes[0];
			if (mpInputStream->eof())
				SetFinished();

			if (rawBytes[0] < 0x80)		//	1-byte code
			{
				if (rawBytes[0])
					resultChar = string (1, char(rawBytes[0]));
			}
			else if (rawBytes[0] < 0xC0)	//	invalid
				PLWARN(::NTV2Line21ChannelToStr(mCaptionChannel) << ": Invalid UTF8 value read from input stream:  " << xHEX0N(uint16_t(rawBytes[0]),2));
			else if (rawBytes[0] < 0xE0)	//	2-byte code
			{
				if (IsFinished())
					PLWARN(::NTV2Line21ChannelToStr(mCaptionChannel) << ": EOF on input stream before reading byte 2 of 2-byte UTF8 character");
				else
				{
					resultChar = char(rawBytes[0]);
					*mpInputStream >> rawBytes[1];
					if (mpInputStream->eof())
						SetFinished ();
					resultChar += char(rawBytes[1]);
					NTV2_ASSERT(resultChar.length() == 2);
				}
			}
			else if (rawBytes[0] < 0xF0)	//	3-byte code
			{
				resultChar = char(rawBytes[0]);
				for (unsigned ndx(1);  ndx <= 2;  ndx++)
				{
					if (IsFinished())
					{
						PLWARN(::NTV2Line21ChannelToStr(mCaptionChannel) << ": EOF on input stream before reading byte " << DEC(ndx + 1) << " of 3-byte UTF8 character");
						resultChar = "";
						break;
					}
					*mpInputStream >> rawBytes[ndx];
					if (mpInputStream->eof())
						SetFinished();
					resultChar += char(rawBytes[ndx]);
				}
				if (!IsFinished())	NTV2_ASSERT(resultChar.length() == 3);
			}
			else if (rawBytes[0] < 0xF8)	//	4-byte code
			{
				resultChar = char(rawBytes[0]);
				for (unsigned ndx(1);  ndx <= 3;  ndx++)
				{
					if (IsFinished())
					{
						PLWARN(::NTV2Line21ChannelToStr(mCaptionChannel) << "; EOF on input stream before reading byte " << DEC(ndx + 1) << " of 4-byte UTF8 character");
						resultChar = "";
						break;
					}
					*mpInputStream >> rawBytes[ndx];
					if (mpInputStream->eof())
						SetFinished();
					resultChar += char(rawBytes[ndx]);
				}
				if (!IsFinished())	NTV2_ASSERT(resultChar.length() == 4);
			}
			else
				PLWARN(::NTV2Line21ChannelToStr(mCaptionChannel) << ": UTF8 byte value not handled:  " << xHEX0N(uint16_t(rawBytes[0]),2));

			#if defined (MEASURE_ACCURACY)
				if (!resultChar.empty ())
					mCharSpeed.Sample ();

				if (AJATime::GetSystemMilliseconds () - gWhenLastDisplay > 1000)
				{
					//	Once per second, see how far off I am from the target character rate, and adjust the sleep time as needed...
					const double	avg		(mCharSpeed.GetAvgMilliSecsBetweenSamples ());
					const double	diff	(avg - mMilliSecsPerChar);
					milliSecsPerChar = milliSecsPerChar - diff;
					if (milliSecsPerChar < 0.0)
						milliSecsPerChar = mMilliSecsPerChar;
					PLDBG(mCharSpeed.GetSampleCountTally() << " " << ::NTV2Line21ChannelToStr(mCaptionChannel)
							<< " chars:  " << mCharSpeed << "     " << milliSecsPerChar << "ms/Char    " << avg << "avg - "
							<< mMilliSecsPerChar << " => " << diff << "diff");
					gWhenLastDisplay = AJATime::GetSystemMilliseconds ();
				}
			#endif	//	MEASURE_ACCURACY

			return resultChar;

		}	//	GetNextCaptionCharacter


		/**
			@brief	Returns the next "word" read from my input stream. If my text input stream has reached
					EOF, I set my "finished" flag and return an empty string. If the returned word is longer
					than my maximum allowable word length, the string is truncated to that maximum length.
			@param[out]	outLineBreak	Returns true if a newline or line break was encountered in the resulting
										caption word;  otherwise returns false.
		**/
		virtual string GetNextCaptionWord (bool & outLineBreak)
		{
			std::locale	loc;
			string		resultWord;
			string		nextChar	(GetNextCaptionCharacter ());

			outLineBreak = false;
			while (!nextChar.empty ())
			{
				if (IsSpaceChar (nextChar) || IsControlChar (nextChar))
				{
					outLineBreak = (nextChar.at (0) == '\n' || nextChar.at (0) == '\r');
					break;	//	Control chars & whitespace break words (HT, LF, CR, etc.) -- but don't include them in the word
				}

				resultWord += nextChar;
				if (IsWordBreakCharacter (nextChar))
					break;	//	Some punctuation breaks word -- include in the word, too

				nextChar = GetNextCaptionCharacter ();
			}	//	loop til break

			return resultWord;

		}	//	GetNextCaptionWord


		/**
			@brief	Returns the next "sentence" read from my input stream. I build the "sentence" by reading "words"
					using my GetNextCaptionWord function. I always try to return a sentence as close to my designated
					maximum sentence length as possible. In Text Mode, there's no limit to the length of the row text.
		**/
		virtual string GetNextCaptionRow (const bool inBreakLinesOnNewLineChars = false)
		{
			string	resultRow;

			//	Build resultRow one word at a time (except Text Mode)...
			while (true)
			{
				if (mIsTextMode)
				{
					//	Special case for Text Mode...
					string	nextChar	(GetNextCaptionCharacter ());
					while (!nextChar.empty ())
					{
						if (nextChar.at (0) == '\n' || nextChar.at (0) == '\r')
							return resultRow;	//	LF and CR will break the row -- but don't include LF/CR in the row text

						resultRow += nextChar;
						nextChar = GetNextCaptionCharacter ();
					}	//	loop til break
					return resultRow;
				}	//	if Text Mode


				//	Any text left over from the last time?
				if (!mLeftoverRowText.empty ())
				{
					//	Use whatever text was left over from the last time...
					if (mLeftoverRowText.length () > mMaxRowCharWidth)
					{
						//	Not all of it will fit. Use whatever will fit and save the rest for later...
						resultRow = mLeftoverRowText.substr (0, mMaxRowCharWidth);
						mLeftoverRowText.erase (0, mMaxRowCharWidth);
						break;	//	Done
					}
					else
					{
						resultRow = mLeftoverRowText;
						mLeftoverRowText.clear ();
					}
				}	//	if any leftover text

				if (resultRow.length () == mMaxRowCharWidth)	//	Completely full?
					break;										//	If so, send it!

				//	ResultRow has room for more.
				//	Get the next word from the caption source...
				bool	lineBreak	(false);
				string	nextWord	(GetNextCaptionWord (lineBreak));
				if (nextWord.empty () && IsFinished ())
					break;

				if (nextWord.empty () && !lineBreak)
					continue;						//	Nothing to add at this time

				//	If there's anything to send, and the word is maxRowWidth or longer, send resultRow now, and save the rest...
				if (!resultRow.empty () && nextWord.length () >= mMaxRowCharWidth)
				{
					mLeftoverRowText = nextWord;	//	Save the word for next time
					break;							//	Send resultRow now
				}

				//	If the word is longer than maxRowWidth...
				if (nextWord.length () >= mMaxRowCharWidth)
				{
					resultRow = nextWord.substr (0, mMaxRowCharWidth);	//	Send whatever will fit
					nextWord.erase (0, mMaxRowCharWidth);
					mLeftoverRowText = nextWord;						//	Save the rest for next time
					break;												//	Send resultRow now
				}

				//	Add the word only if it will fit...
				const size_t	newLength	((resultRow.length () ? resultRow.length () + 1 : 0) + nextWord.length ());
				if (newLength > mMaxRowCharWidth)
				{
					//	Wrap this word onto next line...
					mLeftoverRowText = nextWord;	//	Save the word for next time
					break;							//	Send resultRow now
				}

				if (resultRow.empty ())
					resultRow = nextWord;
				else
					resultRow += " " + nextWord;
				if (inBreakLinesOnNewLineChars && lineBreak)
					break;
			}	//	loop til I have a row to return

			return resultRow;

		}	//	GetNextCaptionRow


		virtual inline bool	IsPlainTextSource (void) const	{return true;}			///< @return	True if I only produce plaintext;  otherwise false.
		virtual inline bool IsFinished (void) const			{return mFinished;}		///< @return	True if I'm finished -- i.e., if I've delivered my last word.
		virtual inline void SetFinished (void)				{mFinished = true;}		///< @brief	Sets my "finished" flag.
		virtual inline void	SetCaptionChannel (const NTV2Line21Channel inCCChannel)	{mCaptionChannel = inCCChannel;}	///< @brief		Sets my caption channel
		virtual inline NTV2Line21Channel GetCaptionChannel (void) const				{return mCaptionChannel;}			///< @return	My caption channel
		virtual inline bool	IsF1Channel(void) const			{return IsField1Line21CaptionChannel(mCaptionChannel);}		///< @return	True if my caption channel is destined for Field1; otherwise false.


		/**
			@brief	Sets my text mode. If I'm in text mode, I don't do any word breaking or line/row truncation.
			@param[in]	inIsTextMode	Specify true if I'm supplying caption data to a text channel (Tx1, Tx2, Tx3 or Tx4).
										Specify false if I'm supplying caption data to a normal caption channel (CC1, CC2, CC3, or CC4).
		**/
		virtual inline void SetTextMode (const bool inIsTextMode)	{mIsTextMode = inIsTextMode;}

	//	Private Instance Methods
	private:
		inline			CaptionSource (const CaptionSource & inCaptionSource)				: mDeleteInputStream (false)	{(void) inCaptionSource;}	//	Hidden
		virtual inline	CaptionSource & operator = (const CaptionSource & inCaptionSource)	{(void) inCaptionSource; return *this;}						//	Hidden


	//	Instance Data
	private:
		istream *			mpInputStream;		///< @brief	My current input file stream
		string				mInputFilePath;		///< @brief	My input file stream's file path (if any)
		double				mCharsPerMinute;	///< @brief	My character emission rate, in characters per minute
		string				mLeftoverRowText;	///< @brief	My row text accumulator
		size_t				mMaxRowCharWidth;	///< @brief	My maximum row width, in characters
		bool				mFinished;			///< @brief	True if I've delivered everything I can
		const bool			mDeleteInputStream;	///< @brief	True if I'm responsible for deleting the input stream
		bool				mIsTextMode;		///< @brief	True if I'm generating Text Mode caption data instead of normal Captions
		double				mMilliSecsPerChar;	///< @brief	Delay between emitting successive characters, in milliseconds
		NTV2Line21Channel	mCaptionChannel;	///< @brief	Used to tag debug output only
		#if defined (MEASURE_ACCURACY)
			Speedometer		mCharSpeed;			///< @brief	Used to measure/adjust the rate at which I generate captions.
		#endif	//	MEASURE_ACCURACY


	//	Private Class Methods
	private:
		static bool		IsWordBreakCharacter (const string & inUTF8Char)
		{
			static const string	wordBreakCharacters	("!#$%&()*+,./:;<=>?@[\\]^_`{|}~. \t");
			if (inUTF8Char.length () != 1)
				return false;
			return wordBreakCharacters.find (inUTF8Char) != string::npos;
		}

		static bool		IsSpaceChar (const string & inUTF8Char)
		{
			//	tab, newline, vertical tab, form feed, carriage return, and space
			if (inUTF8Char.length () != 1)
				return false;
			const unsigned char	c =	UByte(inUTF8Char[0]);
			if (c == 0x09 || c == 0x0A || c == 0x0B || c == 0x0C || c == 0x0D || c == 0x20)
				return true;
			return false;
		}

		static bool		IsControlChar (const string & inUTF8Char)
		{
			//	ASCII characters octal codes 000 through 037, and 177 (DEL)
			if (inUTF8Char.length () != 1)
				return false;
			unsigned char	c = UByte(inUTF8Char[0]);
			if ((c > 0x00 && c < ' ') || c == 0x7F)
				return true;
			return false;
		}

};	//	CaptionSource


class SCCSource : public CaptionSource
{
	public:		//	PUBLIC INSTANCE METHODS
		explicit SCCSource (const double inCharsPerMinute, istream * pInInputStream, const string & inFilePath, const bool inDeleteInputStream = false);
		virtual ~SCCSource ()
		{
		}
		virtual bool IsPlainTextSource (void) const							{return false;}			///< @return	True if I only produce plaintext;  otherwise false.
		virtual bool EnqueueCCDataToFrame (CNTV2CaptionEncoder608Ptr inEncoder, const uint32_t inFrameNum);	///< @brief	Enqueues next-available CC data into the given encoder for the given frame number.
		virtual string GetNextCaptionCharacter (void)						{return string();}		///< @return	Stubbed out -- returns an empty string. (I'm not a plaintext generator.)
		virtual string GetNextCaptionWord (bool & outLineBreak)				{(void) outLineBreak; return string();}	///< @return	Stubbed out -- returns an empty string. (I'm not a plaintext generator.)
		virtual string GetNextCaptionRow (const bool inBreakLines = false)	{(void) inBreakLines; return string();}	///< @return	Stubbed out -- returns an empty string. (I'm not a plaintext generator.)

	private:	//	PRIVATE INSTANCE METHODS
		virtual inline	SCCSource & operator = (const SCCSource & inCaptionSource)	{(void) inCaptionSource; return *this;}	//	Hidden

	private:	//	PRIVATE INSTANCE DATA
		typedef std::vector<uint16_t>		UWords;
		typedef UWords::const_iterator		UWordsConstIter;
		typedef std::map<uint32_t,UWords>	CCDataMap;	//	Mapping: starting frame# to list of CCData words
		typedef CCDataMap::const_iterator	CCDataMapConstIter;

		CCDataMap			mCCDataMap;
		CCDataMapConstIter	mCCDataMapIter;
		size_t				mLastCCDataWordNdx;
		uint32_t			mMaxFrameNum;
};	//	SCCSource



SCCSource::SCCSource (const double inCharsPerMinute, istream * pInInputStream, const string & inFilePath, const bool inDeleteInputStream)
	:	CaptionSource(inCharsPerMinute, pInInputStream, inFilePath, inDeleteInputStream),
		mLastCCDataWordNdx	(0),
		mMaxFrameNum		(0)
{
	NTV2StringList	lines;
	while (!pInInputStream->fail())
	{
		string	line;
		std::getline(*pInInputStream, line);
		aja::strip(line);
		if (!line.empty())
			lines.push_back(line);
	}
	NTV2StringListConstIter	iter(lines.begin());
	if (iter == lines.end())
		{PLFAIL("No lines!");  return;}
	if (iter->find("Scenarist_SCC") == string::npos)
		{PLFAIL("No 'Scenarist_SCC' heading!");  return;}

	AJATimeCode	tc;
	const AJATimeBase	timeBase	(CNTV2DemoCommon::GetAJAFrameRate(NTV2_FRAMERATE_5994));	//	For now, since CEA608 focused
	const bool			isDropFrame	(true);	//	For now, since CEA608 focused
	while (++iter != lines.end())
	{
		const string &	line(*iter);
		if (line.empty())
			continue;	//	empty line
		const size_t tabPos(line.find('\t'));
		if (tabPos == string::npos)
			continue;	//	no tab character

		//	Split line at tab
		const NTV2StringList	tabChunks (aja::split(line, '\t'));
		NTV2_ASSERT(tabChunks.size() > 1);

		//	Parse timecode
		string timecodeChunk(tabChunks.at(0));
		aja::strip(timecodeChunk);
		const NTV2StringList tcPieces(aja::split(timecodeChunk, ':'));
		if (tcPieces.size() < 4)
			{PLWARN(tcPieces.size() << " timecode components separated by ':' fewer than 4");  continue;}
		const uint32_t	tcComponents[4] = {	uint32_t(aja::stoul(tcPieces[0])), uint32_t(aja::stoul(tcPieces[1])),
											uint32_t(aja::stoul(tcPieces[2])), uint32_t(aja::stoul(tcPieces[3])) };
		tc.SetHmsf(tcComponents[0], tcComponents[1], tcComponents[2], tcComponents[3], timeBase, isDropFrame);

		//	Parse UWords
		UWords	ccWords;
		string	ccDataChunk(tabChunks.at(1));
		aja::strip(ccDataChunk);
		const NTV2StringList	ccDataPieces(aja::split(ccDataChunk, ' '));
		for (NTV2StringListConstIter it(ccDataPieces.begin());  it != ccDataPieces.end();  ++it)
		{
			string	ccDataWord(*it);
			aja::strip(ccDataWord);
			if (ccDataWord.find("0x") == string::npos && ccDataWord.find("0X") == string::npos)
				ccDataWord = "0x" + ccDataWord;
			const uint16_t	ccWord(UWord(aja::stoul(ccDataWord, AJA_NULL, 16)));
			ccWords.push_back(ccWord);
		}

		mMaxFrameNum = tc.QueryFrame();
		CCDataMapConstIter	iter(mCCDataMap.find(mMaxFrameNum));
		if (iter != mCCDataMap.end())
			PLWARN("Frame " << mMaxFrameNum << " already present, has " << iter->second.size() << " byte pairs");
		else
			mCCDataMap[mMaxFrameNum] = ccWords;
		PLDBG("Frame " << tc.QueryFrame() << " has " << ccWords.size() << " CC byte pairs");
		mCCDataMapIter = mCCDataMap.begin();
		mLastCCDataWordNdx  =  0;
	}	//	for each line
	if (!mCCDataMap.empty())
		PLINFO("SCC data frames " << mCCDataMap.begin()->first << " thru " << mMaxFrameNum << ", size=" << mCCDataMap.size());
}

bool SCCSource::EnqueueCCDataToFrame (CNTV2CaptionEncoder608Ptr inEncoder, const uint32_t inPlayFrame)
{
	bool	result(false);
	uint32_t	playFrame(mMaxFrameNum ? inPlayFrame % (mMaxFrameNum + 1) : inPlayFrame);
	if (!inEncoder)
		return result;
	if (mCCDataMapIter != mCCDataMap.end()  &&  mLastCCDataWordNdx < mCCDataMapIter->second.size())
	{
		uint32_t	CCFrame(mCCDataMapIter->first);
		if (CCFrame > playFrame)	//	CCFrame ahead of playout frame?
		{
			if (CCFrame == mMaxFrameNum)
				playFrame = CCFrame;	//	This prevents "sticking" on last frame -- force last CC bytes out
			else
			{
				const uint32_t delayFrames(CCFrame - playFrame);
				PLINFO("CCFrame " << DEC(CCFrame) << " > playFrame " << DEC(playFrame) << "|" << DEC(inPlayFrame) << ", delaying " << DEC(delayFrames) << " frames");
				result = true;	//	inEncoder->EnqueueDelay(delayFrames, GetCaptionChannel());
				AJATime::Sleep(delayFrames * 1000 / 30);
				playFrame += delayFrames;
			}
		}
		if (CCFrame <= playFrame)	//	If current CCFrame at or behind playFrame...
		{							//	...then enqueue CCFrame's CCBytes
			PLINFO("CCFrame " << DEC(CCFrame) << " <= playFrame " << DEC(playFrame) << "|" << DEC(inPlayFrame) << ": " << mCCDataMapIter->second);
			do
			{
				const uint16_t	ccWord(mCCDataMapIter->second.at(mLastCCDataWordNdx));
				const UByte		cc1(UByte(ccWord >> 8));
				const UByte		cc2(UByte(ccWord & 0x00FF));
				CaptionData		ccData;
				if (IsF1Channel())	{ccData.f1_char1 = cc1;	ccData.f1_char2 = cc2;	ccData.bGotField1Data = true;}
				else				{ccData.f2_char1 = cc1;	ccData.f2_char2 = cc2;	ccData.bGotField2Data = true;}

				result = inEncoder->EnqueueCaptionData(ccData);
				if (++mLastCCDataWordNdx >= mCCDataMapIter->second.size())
				{
					mLastCCDataWordNdx = 0;
					++mCCDataMapIter;
					if (mCCDataMapIter == mCCDataMap.end())
					{
						SetFinished();
						PLINFO("Finished at playFrame " << DEC(playFrame) << "|" << DEC(inPlayFrame));
						break;	//	Exit do/while loop
					}
				}
			} while (mCCDataMapIter->first == CCFrame);
		}	//	if CCFrame at or behind playFrame
	}
	return result;
}

/**
	@brief		Creates and returns a list of CaptionSource instances to play from a given list
				of paths to text files (which might be empty).
	@param[in]	inFilesToPlay		Specifies a list of paths to text files to be played.
									If empty, use the built-in caption stream.
	@param[in]	inCharsPerMinute	Specifies the desired rate at which caption characters get produced.
	@return		A list of newly-created CaptionSource instances.
**/
static CaptionSourceList GetCaptionSources (const NTV2StringList & inFilesToPlay, const double inCharsPerMinute)
{
	CaptionSourceList	result;
	static istream *	pStdIn			(&cin);
	static size_t		nFilesToPlay	(9999);

	if (nFilesToPlay == 9999)
		nFilesToPlay = inFilesToPlay.size();

	if (nFilesToPlay == 0)
	{
		if (gBuiltInStream.eof())
		{
			//	The built-in caption stream has already been used.
			//	This can happen when running in "loop" mode, so reset it, so it can be used again...
			gBuiltInStream.clear();		//	Clear EOF
			gBuiltInStream.seekg(0);	//	Rewind
		}
		result.push_back(CaptionSourcePtr(new CaptionSource(inCharsPerMinute, &gBuiltInStream, string())));
	}
	else
	{
		for (size_t ndx(0);  ndx < inFilesToPlay.size();  ndx++)
		{
			const string filePath(inFilesToPlay.at(ndx));
			if (filePath == "-")
			{
				if (pStdIn)
				{	//	Add stdin once
					result.push_back(CaptionSourcePtr(new CaptionSource(inCharsPerMinute, pStdIn, string())));
					pStdIn = AJA_NULL;	//	Standard input can only be read from once
				}
				else
					cerr << "## WARNING:  Standard input ('-') can only be specified once" << endl;
			}	//	'-' means "read from standard input"
			else
			{
				ifstream * pFileStream (new ifstream(filePath.c_str()));
				NTV2_ASSERT(pFileStream);
				if (pFileStream->is_open())
				{
					static const string scc(".scc");
					const size_t pos (filePath.rfind(scc));
					if (pos == (filePath.length()-scc.length()))
						result.push_back(CaptionSourcePtr(new SCCSource(inCharsPerMinute, pFileStream, filePath, true)));
					else
						result.push_back(CaptionSourcePtr(new CaptionSource(inCharsPerMinute, pFileStream, filePath, true)));
					PLINFO("Caption file '" << filePath << "' opened");
				}
				else
				{
					PLWARN("Cannot open caption file '" << filePath << "'");
					cerr << "## WARNING:  Cannot play '" << filePath << "'" << endl;
				}
			}	//	else not '-'
		}	//	for each caption file to play
	}	//	else caption file list is not empty

	return result;

}	//	GetCaptionSources



NTV2CCPlayer::NTV2CCPlayer (const CCPlayerConfig & inConfigData)
	:	mConfig					(inConfigData),
		mPlayThread				(),
		mGeneratorThreads		(),
		mDeviceID				(DEVICE_ID_NOTFOUND),
		mSavedTaskMode			(NTV2_DISABLE_TASKS),
		mVideoStandard			(NTV2_STANDARD_INVALID),
		mPlayerQuit				(false),
		mCaptionGeneratorQuit	(false),
		mActiveFrameStores		(),
		mConnections			()
{
	NTV2_ASSERT(!mConfig.fCapChanGenConfigs.empty());
	mGeneratorThreads.resize(size_t(NTV2_CC608_XDS));
	while (mGeneratorThreads.size() < size_t(NTV2_CC608_XDS))
		mGeneratorThreads.push_back(AJAThread());
}	//	constructor


NTV2CCPlayer::~NTV2CCPlayer (void)
{
	//	Stop my playout and producer threads, then destroy them...
	Quit();

	mDevice.UnsubscribeOutputVerticalEvent(mActiveFrameStores);
	if (!mConfig.fDoMultiFormat)
	{
		mDevice.SetEveryFrameServices(mSavedTaskMode);										//	Restore prior service level
		mDevice.ReleaseStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid()));	//	Release the device
	}

}	//	destructor


void NTV2CCPlayer::Quit (const bool inQuitImmediately)
{
	//	Kill the caption generators first...
	mCaptionGeneratorQuit = true;
	while (!mGeneratorThreads.empty())
	{
		while (mGeneratorThreads.back().Active())
			AJATime::Sleep(10);
		mGeneratorThreads.pop_back();
	}

	if (m608Encoder)
	{
		if (inQuitImmediately)
		{
			PLDBG("Quit immediate -- flushing");
			m608Encoder->Flush();					//	Immediately flush all queued messages
		}
		else
		{
			PLDBG("Quit non-immediate -- waiting for queue to drain");
			while (m608Encoder->GetQueuedByteCount(NTV2_CC608_Field1)  ||  m608Encoder->GetQueuedByteCount(NTV2_CC608_Field2))
				AJATime::Sleep(10);				//	Wait for PlayThread encoder's message queues to drain as PlayThread continues
			//	This Sleep isn't necessary, but if not done, the last captions (or Erase) won't get a chance to be seen on downstream
			//	devices (because the AJA device will get released, and the retail services will change the device configuration).
			AJATime::Sleep(2000);
		}
	}

	mPlayerQuit = true;
	while (mPlayThread.Active())
		AJATime::Sleep(10);
	mDevice.RemoveConnections(mConnections);

}	//	Quit


bool NTV2CCPlayer::DeviceAncExtractorIsAvailable (void)
{
	UWord	majorVersion (0),	minorVersion (0),	pointVersion (0),	buildNumber (0);
	mDevice.GetDriverVersionComponents (majorVersion, minorVersion, pointVersion, buildNumber);
	//	Device Anc extraction requires driver version 12.3 minimum  (or 0.0.0.0 for internal development)...
	if ((majorVersion == 12 && minorVersion >= 3) || (majorVersion >= 13) || (majorVersion == 0 && minorVersion == 0 && pointVersion == 0 && buildNumber == 0))
		//	The device must also support it...
		if (mDevice.features().CanDoCustomAnc())
			//	And perhaps even do firmware version/date checks??
			return true;
	return false;
}


AJAStatus NTV2CCPlayer::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);
	CNTV2DemoCommon::SetDefaultPageSize();	//	Set host-specific page size

	//	Any AJA devices out there?
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}
	mDeviceID = mDevice.GetDeviceID();	//	Keep this ID handy -- it's used frequently

    if (!mDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}
	if (!mDevice.features().CanDoPlayback())
		{cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' is capture-only" << endl;  return AJA_STATUS_FEATURE;}

	if (!mConfig.fDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid())))
		{
			cerr << "## ERROR:  Cannot acquire '" << mDevice.GetDisplayName() << "' because another app owns it" << endl;
			return AJA_STATUS_BUSY;		//	Some other app owns the device
		}
		mDevice.GetEveryFrameServices(mSavedTaskMode);	//	Save the current task mode
	}
	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);		//	Set OEM service level

#if defined(_DEBUG)
	if (mConfig.fForceRTP & BIT(2))
	{	//	Hack -- force device to pretend it's a KonaIP2110
		NTV2Buffer	pDevice (&mDevice, sizeof(CNTV2Card));
		ULWordSequence	U32s (pDevice.GetU32s(0, pDevice.GetByteCount()/sizeof(uint32_t)));
		for (unsigned ndx(0);  ndx < U32s.size();  ndx++)
			if (U32s.at(ndx) == ULWord(mDeviceID))
			{	//	by patching its _boardID member...
				mDeviceID = DEVICE_ID_KONAIP_2110;
				U32s[ndx] = ULWord(mDeviceID);
				pDevice.PutU32s(U32s);
				break;
			}
	}
#endif	//	defined(_DEBUG)

	if (mDevice.features().CanDoMultiFormat()  &&  mConfig.fDoMultiFormat)
		mDevice.SetMultiFormatMode(true);
	else if (mDevice.features().CanDoMultiFormat())
		mDevice.SetMultiFormatMode(false);

	if (NTV2_IS_VANCMODE_OFF(mConfig.fVancMode))		//	if user didn't use --vanc option...
		if (!DeviceAncExtractorIsAvailable())			//	and anc extractor isn't available...
			mConfig.fVancMode = NTV2_VANCMODE_TALLER;	//	then enable Vanc anyway

	//	Set up the device video config...
	status = SetUpOutputVideo();
	if (AJA_FAILURE(status))
		return status;

	//	Set up my background pattern buffer...
	status = SetUpBackgroundPatternBuffer();
	if (AJA_FAILURE(status))
		return status;

	//	Set up device signal routing...
	status = RouteOutputSignal();
	if (AJA_FAILURE(status))
		return status;

	#if defined(_DEBUG)
		cerr << mConfig << endl;
	#endif	//	defined(_DEBUG)
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2CCPlayer::SetUpBackgroundPatternBuffer (void)
{
	//	Generate the test pattern...
	NTV2TestPatternGen			testPatternGen;
	const NTV2FormatDescriptor	formatDesc	(mConfig.fVideoFormat, mConfig.fPixelFormat, mConfig.fVancMode);
	testPatternGen.setVANCToLegalBlack(formatDesc.IsVANC());	//	Clear the VANC region to legal black if needed

	//	Allocate and clear the host video buffer memory...
	mVideoBuffer.Allocate(formatDesc.GetVideoWriteSize());

	if (!testPatternGen.DrawTestPattern (mConfig.fTestPatternName, formatDesc, mVideoBuffer))
	{
		cerr << "## ERROR:  DrawTestPattern failed, formatDesc: " << formatDesc << endl;
		return AJA_STATUS_FAIL;
	}

	//	Burn static info into the test pattern...
	const string strVideoFormat (CNTV2DemoCommon::StripFormatString (::NTV2VideoFormatToString(mConfig.fVideoFormat)));
	{	ostringstream oss;
		oss	<< setw(32) << left << string("CCPlayer ") + strVideoFormat + string(formatDesc.IsVANC() ? " VANC" : "");
		CNTV2CaptionRenderer::BurnString (oss.str(), NTV2Line21Attributes(NTV2_CC608_White, NTV2_CC608_Cyan), mVideoBuffer, formatDesc, 1, 1);	//	R1C1
	}
	{	ostringstream oss;
		oss	<< formatDesc.GetRasterWidth() << "Wx" << formatDesc.GetFullRasterHeight() << "H  "
			<< ::NTV2FrameBufferFormatToString(mConfig.fPixelFormat, true) << string(20, ' ');
		CNTV2CaptionRenderer::BurnString (oss.str(), NTV2Line21Attributes(NTV2_CC608_White, NTV2_CC608_Cyan), mVideoBuffer, formatDesc, 2, 1);	//	R2C1
	}
	return AJA_STATUS_SUCCESS;

}	//	SetUpBackgroundPatternBuffer


AJAStatus NTV2CCPlayer::SetUpOutputVideo (void)
{
	//	Preflight checks...
	if (mDevice.features().GetNumVideoOutputs() < 1)
		{cerr << "## ERROR:  Device cannot playout" << endl;  return AJA_STATUS_UNSUPPORTED;}
	if ((mConfig.fOutputChannel == NTV2_CHANNEL1)  &&  !mDevice.features().CanDoFrameStore1Display())
		{cerr << "## ERROR:  Device cannot playout thru FrameStore 1" << endl;  return AJA_STATUS_UNSUPPORTED;}
	if (!NTV2_OUTPUT_DEST_IS_SDI(mConfig.fOutputDest))
	{
		cerr << "## ERROR:  CCPlayer uses SDI only, not '" << ::NTV2OutputDestinationToString(mConfig.fOutputDest,true) << "'" << endl;
		return AJA_STATUS_UNSUPPORTED;
	}
	if (!mDevice.features().CanDoOutputDestination(mConfig.fOutputDest))
	{
		cerr << "## ERROR:  No such output connector '" << ::NTV2OutputDestinationToString(mConfig.fOutputDest,true) << "'" << endl;
		return AJA_STATUS_UNSUPPORTED;
	}
	if (NTV2_IS_QUAD_FRAME_FORMAT(mConfig.fVideoFormat) && !mDevice.features().CanDo12gRouting())
	{
		if (!mConfig.fDoTsiRouting  &&  mConfig.fOutputChannel != NTV2_CHANNEL1  &&  mConfig.fOutputChannel != NTV2_CHANNEL5)
		{
			cerr << "## ERROR:  Quad-frame format must use Ch1|Ch5, not '" << ::NTV2ChannelToString(mConfig.fOutputChannel, true) << "'" << endl;
			return AJA_STATUS_BAD_PARAM;
		}
		else if (mConfig.fDoTsiRouting  &&  (mConfig.fOutputChannel & 1))
		{
			cerr << "## ERROR:  UHD/4K TSI must use Ch[1|3|5|7], not '" << ::NTV2ChannelToString(mConfig.fOutputChannel, true) << "'" << endl;
			return AJA_STATUS_BAD_PARAM;
		}
		const NTV2Channel sdiChan(::NTV2OutputDestinationToChannel(mConfig.fOutputDest));
		if (!mConfig.fDoTsiRouting  &&  sdiChan != NTV2_CHANNEL1  &&  sdiChan != NTV2_CHANNEL5)
		{
			cerr << "## ERROR:  UHD/4K Squares must use SDIOut[1|5], not '" << ::NTV2ChannelToString(mConfig.fOutputChannel, true) << "'" << endl;
			return AJA_STATUS_BAD_PARAM;
		}
		else if (mConfig.fDoTsiRouting  &&  (sdiChan & 1))
		{
			cerr << "## ERROR:  UHD/4K TSI must use SDIOut[1|3|5|7], not '" << ::NTV2ChannelToString(mConfig.fOutputChannel, true) << "'" << endl;
			return AJA_STATUS_BAD_PARAM;
		}
	}
	else
		mConfig.fDoTsiRouting = false;	//	Allows RouteOutputSignal to cheat
	if (NTV2_IS_QUAD_QUAD_FORMAT(mConfig.fVideoFormat))
		{cerr << "## ERROR:  CCPlayer doesn't yet support UHD2/8K formats" << endl;  return AJA_STATUS_UNSUPPORTED;}
	if (NTV2_IS_625_FORMAT(mConfig.fVideoFormat))
		cerr << "## WARNING:  SD 625/PAL not supported -- but will insert CEA608 caption packets anyway" << endl;
	if ((NTV2_IS_4K_VIDEO_FORMAT(mConfig.fVideoFormat) || NTV2_IS_QUAD_QUAD_FORMAT(mConfig.fVideoFormat)))
		if (NTV2_IS_VANCMODE_ON(mConfig.fVancMode))
			{cerr << "## ERROR:  Cannot use VANC mode with UHD/4K/UHD2/8K formats" << endl;  return AJA_STATUS_UNSUPPORTED;}
	if (NTV2_IS_SD_VIDEO_FORMAT(mConfig.fVideoFormat)  &&  !mConfig.fSuppressLine21)
		if (mConfig.fPixelFormat != NTV2_FBF_8BIT_YCBCR  &&  mConfig.fPixelFormat != NTV2_FBF_10BIT_YCBCR)
			{cerr << "## ERROR:  SD/line21 in " << ::NTV2FrameBufferFormatToString(mConfig.fPixelFormat) << "' not '2vuy'|'v210'" << endl;  return AJA_STATUS_UNSUPPORTED;}
	if (UWord(mConfig.fOutputChannel) >= mDevice.features().GetNumFrameStores())
		{cerr << "## ERROR:  Device has " << DEC(mDevice.features().GetNumFrameStores()) << " FrameStore(s), no channel " << DEC(mConfig.fOutputChannel+1) << endl;  return AJA_STATUS_UNSUPPORTED;}

	const NTV2FrameRate	frameRate(::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat));
	if (!NTV2_IS_SD_VIDEO_FORMAT(mConfig.fVideoFormat)  &&  !mConfig.fSuppress708)
		if (frameRate == NTV2_FRAMERATE_2500  ||  frameRate == NTV2_FRAMERATE_5000)
			{cerr << "## ERROR:  CEA708 CDPs can't accommodate CEA608 captions for " << ::NTV2FrameRateToString(frameRate) << endl;  return AJA_STATUS_UNSUPPORTED;}

	//
	//	Enable the required framestore(s)...
	//
	if (NTV2_IS_QUAD_FRAME_FORMAT(mConfig.fVideoFormat))
		mActiveFrameStores = ::NTV2MakeChannelSet(mConfig.fOutputChannel, mConfig.fDoTsiRouting ? 2 : 4);
	else
		mActiveFrameStores.insert(mConfig.fOutputChannel);
	mDevice.EnableChannels (mActiveFrameStores,
							!mConfig.fDoMultiFormat);	//	Disable other channels if not MultiFormat mode

	//
	//	Set video format/standard...
	//
	if (!mDevice.features().CanDoVideoFormat(mConfig.fVideoFormat))
		{cerr << "## ERROR:  '" << ::NTV2VideoFormatToString(mConfig.fVideoFormat) << "' not supported" << endl;  return AJA_STATUS_UNSUPPORTED;}
	mVideoStandard = ::GetNTV2StandardFromVideoFormat(mConfig.fVideoFormat);
	mDevice.SetVideoFormat (mActiveFrameStores, mConfig.fVideoFormat, AJA_RETAIL_DEFAULT);
	if (!NTV2_IS_QUAD_FRAME_FORMAT(mConfig.fVideoFormat))
		{}
	else if (mConfig.fDoTsiRouting)
		mDevice.SetTsiFrameEnable(true, mConfig.fOutputChannel);
	else
		mDevice.Set4kSquaresEnable(true, mConfig.fOutputChannel);

	//
	//	Set frame buffer pixel format...
	//
	if (!mDevice.features().CanDoFrameBufferFormat (mConfig.fPixelFormat))
		{cerr << "## ERROR:  '" << ::NTV2FrameBufferFormatToString(mConfig.fPixelFormat) << "' not supported" << endl;  return AJA_STATUS_UNSUPPORTED;}
	if (NTV2_IS_VANCMODE_ON(mConfig.fVancMode)  &&  ::IsRGBFormat(mConfig.fPixelFormat) != mConfig.fDoRGBOnWire)
		{cerr << "## ERROR:  Routing thru CSC may corrupt VANC in frame buffer" << endl;  return AJA_STATUS_UNSUPPORTED;}
	mDevice.SetFrameBufferFormat (mActiveFrameStores, mConfig.fPixelFormat);
	if (NTV2_IS_VANCMODE_OFF(mConfig.fVancMode))
		if (mConfig.fOutputChannel != ::NTV2OutputDestinationToChannel(mConfig.fOutputDest))
		{
			cerr << "## ERROR:  FrameStore" << DEC(mConfig.fOutputChannel+1)
					<< " doesn't correlate with SDIOut" << DEC(::NTV2OutputDestinationToChannel(mConfig.fOutputDest)+1)
					<< " -- Anc inserter requires both to match, output signal won't have captions" << endl;
			return AJA_STATUS_FAIL;
		}

	//
	//	Enable VANC only if device has no Anc inserters, or if --vanc specified...
	//
	if (NTV2_IS_QUAD_FRAME_FORMAT(mConfig.fVideoFormat))
		NTV2_ASSERT(NTV2_IS_VANCMODE_OFF(mConfig.fVancMode));
	mDevice.SetVANCMode (mActiveFrameStores, mConfig.fVancMode);
	if (!NTV2_IS_QUAD_FRAME_FORMAT(mConfig.fVideoFormat))
		if (NTV2_IS_VANCMODE_ON(mConfig.fVancMode))
			if (::Is8BitFrameBufferFormat(mConfig.fPixelFormat))
				mDevice.SetVANCShiftMode (mConfig.fOutputChannel, NTV2_VANCDATA_8BITSHIFT_ENABLE);	//	8-bit FBFs require VANC bit shift

	//
	//	Create the caption encoders...
	//
	if (!CNTV2CaptionEncoder608::Create(m608Encoder))
		{cerr << "## ERROR:  Cannot create 608 encoder" << endl;  return AJA_STATUS_MEMORY;}
	if (!CNTV2CaptionEncoder708::Create(m708Encoder))
		{cerr << "## ERROR:  Cannot create 708 encoder" << endl;  return AJA_STATUS_MEMORY;}

	mDevice.SetReference (mDevice.features().CanDo2110() ? NTV2_REFERENCE_SFP1_PTP : NTV2_REFERENCE_FREERUN);

	//
	//	Subscribe to the output interrupt(s)...
	//
	mDevice.SubscribeOutputVerticalEvent(mActiveFrameStores);

	cerr	<< "## NOTE:  Generating '" << ::NTV2VideoFormatToString(mConfig.fVideoFormat)
			<< "' using " << (NTV2_IS_VANCMODE_ON(mConfig.fVancMode) ? "VANC" : "device Anc inserter")
			<< " on '" << mDevice.GetDisplayName() << "' to " << ::NTV2OutputDestinationToString(mConfig.fOutputDest)
			<< (mConfig.fDoRGBOnWire ? " (DL-RGB)" : "")
			<< " from FrameStore" << DEC(mConfig.fOutputChannel+1)
			<< " using " << ::NTV2FrameBufferFormatToString(mConfig.fPixelFormat) << endl;
	return AJA_STATUS_SUCCESS;

}	//	SetUpOutputVideo


AJAStatus NTV2CCPlayer::RouteOutputSignal (void)
{
	const bool			isRGBFBF	(::IsRGBFormat(mConfig.fPixelFormat));
	const bool			isRGBWire	(mConfig.fDoRGBOnWire);	//	RGB-over-SDI?
	const bool			isQuadFmt	(NTV2_IS_QUAD_FRAME_FORMAT(mConfig.fVideoFormat));
	const bool			is4KHFR		(NTV2_IS_HFR_STANDARD(mVideoStandard));
	const NTV2Channel	sdiOutput	(::NTV2OutputDestinationToChannel(mConfig.fOutputDest));
	NTV2ChannelSet		sdiOuts		(::NTV2MakeChannelSet(sdiOutput, UWord(mActiveFrameStores.size())));
	NTV2ChannelList		sdiOutputs	(::NTV2MakeChannelList(sdiOuts));
	NTV2ChannelList		frameStores	(::NTV2MakeChannelList(mActiveFrameStores));
	NTV2ChannelList		tsiMuxes, cscs;
	mConnections.clear();

	//	Does device have RGB conversion capability for the desired channel?
	if (isRGBFBF != isRGBWire	//	if any CSC(s) are needed
		&& UWord(mConfig.fOutputChannel) > mDevice.features().GetNumCSCs())
			{cerr << "## ERROR:  No CSC for channel " << (mConfig.fOutputChannel+1) << endl;  return AJA_STATUS_UNSUPPORTED;}
	//*UNCOMMENT TO SHOW ROUTING PROGRESS WHILE DEBUGGING*/mDevice.ClearRouting();

	mDevice.SetSDITransmitEnable(sdiOuts, true);
	if (isRGBFBF && isRGBWire)
	{
		if (!mConfig.fDoTsiRouting)	//	RGBFrameStore ==> DLOut ==> SDIOut
			for (size_t ndx(0);  ndx < sdiOutputs.size();  ndx++)
			{	NTV2Channel frmSt(frameStores.at(ndx)), sdiOut(sdiOutputs.at(ndx));
				mConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut, /*isDS2*/false),  ::GetDLOutOutputXptFromChannel(sdiOut, /*isDS2*/false)));
				mConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut, /*isDS2*/true),  ::GetDLOutOutputXptFromChannel(sdiOut, /*isDS2*/true)));
				mConnections.insert(NTV2XptConnection(::GetDLOutInputXptFromChannel(sdiOut),  ::GetFrameBufferOutputXptFromChannel(frmSt,  true/*isRGB*/,  false/*is425*/)));
				//*UNCOMMENT TO SHOW ROUTING PROGRESS WHILE DEBUGGING*/mDevice.ApplySignalRoute(mConnections);
				//  Disable SDI output conversions
				mDevice.SetSDIOutLevelAtoLevelBConversion(sdiOut, false);
				mDevice.SetSDIOutRGBLevelAConversion(sdiOut, false);
			}
		else
		{								//	RGBFrameStore ==> 425MUX ==> 2x DLOut ==> 2x SDIOut
			sdiOuts		= ::NTV2MakeChannelSet(sdiOutput, UWord(2*mActiveFrameStores.size()));
			sdiOutputs	= ::NTV2MakeChannelList(sdiOuts);
			tsiMuxes = CNTV2DemoCommon::GetTSIMuxesForFrameStore(mDevice, frameStores.at(0), UWord(frameStores.size()*2));
			mDevice.SetSDITransmitEnable(sdiOuts, true);	//	Gotta do this again, since sdiOuts changed
			for (size_t ndx(0);  ndx < sdiOutputs.size();  ndx++)
			{	NTV2Channel frmSt(frameStores.at(ndx/2)), tsiMux(tsiMuxes.at(ndx/2)), sdiOut(sdiOutputs.at(ndx));
				mConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut, /*isDS2*/false),	::GetDLOutOutputXptFromChannel(sdiOut, /*isDS2*/false)));
				mConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut, /*isDS2*/true),	::GetDLOutOutputXptFromChannel(sdiOut, /*isDS2*/true)));
				mConnections.insert(NTV2XptConnection(::GetDLOutInputXptFromChannel(sdiOut),			::GetTSIMuxOutputXptFromChannel(tsiMux, /*isLinkB*/(ndx & 1) > 0, /*isRGB*/true)));
				mConnections.insert(NTV2XptConnection(::GetTSIMuxInputXptFromChannel(tsiMux,/*linkB?*/false), ::GetFrameBufferOutputXptFromChannel(frmSt,  true/*isRGB*/,  false/*is425*/)));
				mConnections.insert(NTV2XptConnection(::GetTSIMuxInputXptFromChannel(tsiMux,/*linkB?*/true), ::GetFrameBufferOutputXptFromChannel(frmSt,  true/*isRGB*/,  true/*is425*/)));
				//*UNCOMMENT TO SHOW ROUTING PROGRESS WHILE DEBUGGING*/mDevice.ApplySignalRoute(mConnections);
			}
		}
	}
	else if (isRGBFBF && !isRGBWire)
	{
		if (!mConfig.fDoTsiRouting)	//	RGBFrameStore ==> CSC ==> SDIOut
			for (size_t ndx(0);  ndx < sdiOutputs.size();  ndx++)
			{	NTV2Channel frmSt(frameStores.at(ndx)), sdiOut(sdiOutputs.at(ndx));
				mConnections.insert(NTV2XptConnection(::GetCSCInputXptFromChannel(frmSt),
														::GetFrameBufferOutputXptFromChannel(frmSt, /*isRGB*/true, /*is425*/false)));
				mConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut),
														::GetCSCOutputXptFromChannel(frmSt)));
				//*UNCOMMENT TO SHOW ROUTING PROGRESS WHILE DEBUGGING*/mDevice.ApplySignalRoute(mConnections);
				//  Disable SDI output conversions
				mDevice.SetSDIOutLevelAtoLevelBConversion(sdiOut, false);
				mDevice.SetSDIOutRGBLevelAConversion(sdiOut, false);
			}
		else
		{	//	TSI			//	RGBFrameStore  ==>  425MUX  ==>  LFR: 2 x CSC ==> 2 x SDIOut    HFR: 4 x CSC ==> 4 x SDI
			sdiOuts		= ::NTV2MakeChannelSet(sdiOutput, is4KHFR ? 4 : UWord(mActiveFrameStores.size()));
			sdiOutputs	= ::NTV2MakeChannelList(sdiOuts);
			tsiMuxes	= CNTV2DemoCommon::GetTSIMuxesForFrameStore(mDevice, frameStores.at(0), UWord(frameStores.size()));
			cscs		= ::NTV2MakeChannelList(mConfig.fOutputChannel > NTV2_CHANNEL4 ? NTV2_CHANNEL5 : NTV2_CHANNEL1, isQuadFmt ? 4 : 2);
			//cerr	<< " FrmSt: "	<< ::NTV2ChannelListToStr(frameStores)	<< endl		<< " TSIMx: "	<< ::NTV2ChannelListToStr(tsiMuxes)		<< endl
			//		<< "  CSCs: "	<< ::NTV2ChannelListToStr(cscs)			<< endl		<< "SDIOut: "	<< ::NTV2ChannelListToStr(sdiOutputs)	<< endl;
			mDevice.SetSDITransmitEnable(sdiOuts, true);	//	Do this again, since sdiOuts changed
			for (size_t ndx(0);  ndx < cscs.size();  ndx++)
			{	NTV2Channel frmSt (frameStores.at(ndx/2)),	tsiMux (tsiMuxes.at(ndx/2)),
							sdiOut (sdiOutputs.at(is4KHFR ? ndx : ndx/2)),	csc (cscs.at(ndx));
				mConnections.insert(NTV2XptConnection(::GetTSIMuxInputXptFromChannel(tsiMux,/*linkB?*/ndx & 1),
													::GetFrameBufferOutputXptFromChannel(frmSt,  true/*isRGB*/,  ndx & 1/*is425*/)));
				mConnections.insert(NTV2XptConnection(::GetCSCInputXptFromChannel(csc),
													::GetTSIMuxOutputXptFromChannel(tsiMux, /*isLinkB*/ndx & 1, /*isRGB*/true)));
				mConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut, /*isDS2*/is4KHFR ? false : ndx & 1),
													::GetCSCOutputXptFromChannel(cscs.at(ndx))));
				//*UNCOMMENT TO SHOW ROUTING PROGRESS WHILE DEBUGGING*/mDevice.ApplySignalRoute(mConnections);
			}
		}
	}
	else if (!isRGBFBF && isRGBWire)
	{
		if (!mConfig.fDoTsiRouting)	//	YUVFrameStore ==> CSC ==> DLOut ==> SDIOut
			for (size_t ndx(0);  ndx < sdiOutputs.size();  ndx++)
			{	NTV2Channel frmSt(frameStores.at(ndx)), sdiOut(sdiOutputs.at(ndx));
				mConnections.insert(NTV2XptConnection(::GetCSCInputXptFromChannel(frmSt, /*isKeyInput*/false),	::GetFrameBufferOutputXptFromChannel(frmSt, false/*isRGB*/, false/*is425*/)));
				mConnections.insert(NTV2XptConnection(::GetDLOutInputXptFromChannel(sdiOut),					::GetCSCOutputXptFromChannel(frmSt, /*isKey*/false, /*isRGB*/true)));
				mConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut, /*isDS2*/false),			::GetDLOutOutputXptFromChannel(sdiOut, /*isDS2*/false)));
				mConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut, /*isDS2*/true),			::GetDLOutOutputXptFromChannel(sdiOut, /*isDS2*/true)));
				//*UNCOMMENT TO SHOW ROUTING PROGRESS WHILE DEBUGGING*/mDevice.ApplySignalRoute(mConnections);
				//  Disable SDI output conversions
				mDevice.SetSDIOutLevelAtoLevelBConversion(sdiOut, false);
				mDevice.SetSDIOutRGBLevelAConversion(sdiOut, false);
			}
		else
		{								//	YUVFrameStore ==> 425MUX ==> 2x CSC ==> 2x DLOut ==> 2x SDIOut
			sdiOuts		= ::NTV2MakeChannelSet(sdiOutput, UWord(2*mActiveFrameStores.size()));
			sdiOutputs	= ::NTV2MakeChannelList(sdiOuts);
			tsiMuxes = CNTV2DemoCommon::GetTSIMuxesForFrameStore(mDevice, frameStores.at(0), UWord(frameStores.size()*2));
			mDevice.SetSDITransmitEnable(sdiOuts, true);	//	Gotta do this again, since sdiOuts changed
			for (size_t ndx(0);  ndx < sdiOutputs.size();  ndx++)
			{	NTV2Channel frmSt(frameStores.at(ndx/2)), tsiMux(tsiMuxes.at(ndx/2)), sdiOut(sdiOutputs.at(ndx));
				mConnections.insert(NTV2XptConnection(::GetTSIMuxInputXptFromChannel(tsiMux,/*linkB?*/false),	::GetFrameBufferOutputXptFromChannel(frmSt,  false/*isRGB*/,  false/*is425*/)));
				mConnections.insert(NTV2XptConnection(::GetTSIMuxInputXptFromChannel(tsiMux,/*linkB?*/true),	::GetFrameBufferOutputXptFromChannel(frmSt,  false/*isRGB*/,  true/*is425*/)));
				mConnections.insert(NTV2XptConnection(::GetCSCInputXptFromChannel(sdiOut, /*isKeyInput*/false),	::GetTSIMuxOutputXptFromChannel(tsiMux, /*linkB*/(ndx & 1) > 0,/*isRGB*/false)));
				mConnections.insert(NTV2XptConnection(::GetDLOutInputXptFromChannel(sdiOut),			::GetCSCOutputXptFromChannel(NTV2Channel(sdiOut),/*isKey*/false,/*isRGB*/true)));
				mConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut, /*isDS2*/false),	::GetDLOutOutputXptFromChannel(sdiOut, /*isDS2*/false)));
				mConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut, /*isDS2*/true),	::GetDLOutOutputXptFromChannel(sdiOut, /*isDS2*/true)));
				//*UNCOMMENT TO SHOW ROUTING PROGRESS WHILE DEBUGGING*/mDevice.ApplySignalRoute(mConnections);
			}
		}
	}
	else	//	!isRGBFBF && !isRGBWire
	{
		if (!mConfig.fDoTsiRouting)	//	YUVFrameStore  ==>  SDIOut
			for (size_t ndx(0);  ndx < sdiOutputs.size();  ndx++)
			{	NTV2Channel frmSt(frameStores.at(ndx)), sdiOut(sdiOutputs.at(ndx));
				mConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut),
														::GetFrameBufferOutputXptFromChannel(frmSt)));
				//*UNCOMMENT TO SHOW ROUTING PROGRESS WHILE DEBUGGING*/mDevice.ApplySignalRoute(mConnections);
				//  Disable SDI output conversions
				mDevice.SetSDIOutLevelAtoLevelBConversion(sdiOut, false);
				mDevice.SetSDIOutRGBLevelAConversion(sdiOut, false);
			}
		else
		{	// TSI						//	YUVFrameStore ==> 425MUX ==> SDIOut (LFR: 2 x DS1&DS2, 4KHFR: 4 x DS1)
			sdiOuts		= ::NTV2MakeChannelSet(sdiOutput, is4KHFR ? 4 : UWord(mActiveFrameStores.size()));
			sdiOutputs	= ::NTV2MakeChannelList(sdiOuts);
			tsiMuxes	= CNTV2DemoCommon::GetTSIMuxesForFrameStore(mDevice, frameStores.at(0), UWord(frameStores.size()));
			//cerr	<< "FrameStores: "	<< ::NTV2ChannelListToStr(frameStores)	<< endl		<< "SDIOutputs: "	<< ::NTV2ChannelListToStr(sdiOutputs)	<< endl
			//		<< "TSIMuxers: "	<< ::NTV2ChannelListToStr(tsiMuxes)		<< endl;
			mDevice.SetSDITransmitEnable(sdiOuts, true);	//	Do this again, since sdiOuts changed
			for (size_t ndx(0);  ndx < sdiOutputs.size();  ndx++)
			{	NTV2Channel frmSt(frameStores.at(is4KHFR ? ndx/2 : ndx)),
							tsiMux(tsiMuxes.at(is4KHFR ? ndx/2 : ndx)),
							sdiOut(sdiOutputs.at(ndx));
				mConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut, /*isDS2*/false),
													::GetTSIMuxOutputXptFromChannel(tsiMux, /*linkB?*/is4KHFR && (ndx & 1), /*RGB?*/false)));
				if (!is4KHFR)
					mConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut, /*isDS2*/true),
														::GetTSIMuxOutputXptFromChannel(tsiMux, /*linkB?*/true, /*RGB?*/false)));
				mConnections.insert(NTV2XptConnection(::GetTSIMuxInputXptFromChannel(tsiMux,/*linkB?*/is4KHFR && (ndx & 1)),
													::GetFrameBufferOutputXptFromChannel(frmSt, /*RGB?*/false,  /*425?*/is4KHFR && (ndx & 1))));
				if (!is4KHFR)
					mConnections.insert(NTV2XptConnection(::GetTSIMuxInputXptFromChannel(tsiMux,/*linkB?*/true),
														::GetFrameBufferOutputXptFromChannel(frmSt, /*RGB?*/false,  /*425?*/true)));
				//*UNCOMMENT TO SHOW ROUTING PROGRESS WHILE DEBUGGING*/mDevice.ApplySignalRoute(mConnections);
			}
		}
	}
	return mDevice.ApplySignalRoute(mConnections, /*replaceExistingRouting?*/!mConfig.fDoMultiFormat) ? AJA_STATUS_SUCCESS : AJA_STATUS_UNSUPPORTED;

}	//	RouteOutputSignal


AJAStatus NTV2CCPlayer::Run (void)
{
	//	Start the threads...
	StartPlayoutThread();
	StartCaptionGeneratorThreads();
	return AJA_STATUS_SUCCESS;

}	//	Run


//////////////////////////////////////////////
//	Caption generator thread
//////////////////////////////////////////////

class CapGenStartInfo
{	public:
		NTV2CCPlayer *		fpPlayer;
		NTV2Line21Channel	fCapChannel;
		uint32_t			fSpare;
		explicit CapGenStartInfo (NTV2CCPlayer * pPlayer, const NTV2Line21Channel inCapChan)
			:	fpPlayer(pPlayer), fCapChannel(inCapChan)
			{}
};


void NTV2CCPlayer::StartCaptionGeneratorThreads ()
{
	//	Create and start the caption generator threads, one per caption channel...
	for (CaptionChanGenMapCIter it(mConfig.fCapChanGenConfigs.begin());  it != mConfig.fCapChanGenConfigs.end();  ++it)
	{
		const CCGenConfig &	genConfig (it->second);
		AJAThread &	genThread (mGeneratorThreads.at(size_t(genConfig.fCaptionChannel)));
		//	The generator thread needs to know the NTV2CCPlayer instance, and the caption channel:
		genThread.Attach(CaptionGeneratorThreadStatic,
						reinterpret_cast<void*>(new CapGenStartInfo(this, genConfig.fCaptionChannel)));
		genThread.Start();
	}
}	//	StartCaptionGeneratorThreads


void NTV2CCPlayer::CaptionGeneratorThreadStatic (AJAThread * pThread, void * pContext)		//	STATIC
{	(void) pThread;
	//	Extract all the startup info I need...
	const CapGenStartInfo * pStartInfo (reinterpret_cast<CapGenStartInfo*>(pContext));
	NTV2_ASSERT(pStartInfo);
	NTV2CCPlayer * pApp (pStartInfo->fpPlayer);
	NTV2_ASSERT(pApp);
	const NTV2Line21Channel captionChannel (pStartInfo->fCapChannel);
	NTV2_ASSERT(IsValidLine21Channel(captionChannel));
	delete pStartInfo;	//	Done with it, free it

	//	Start generating captions for the given caption channel...
	pApp->GenerateCaptions(captionChannel);

}	//	CaptionGeneratorThreadStatic


void NTV2CCPlayer::GenerateCaptions (const NTV2Line21Channel inCCChannel)
{
	CaptionChanGenMapCIter	iter(mConfig.fCapChanGenConfigs.find(inCCChannel));
	NTV2_ASSERT(iter != mConfig.fCapChanGenConfigs.end());

	const CCGenConfig &		ccGenConfig			(iter->second);
	const NTV2Line21Mode	captionMode			(ccGenConfig.fCaptionMode);			//	My caption mode (paint-on, pop-on, roll-up, etc.)
	const double			charsPerMinute		(ccGenConfig.fCharsPerMinute);		//	Desired caption rate (in characters per minute)
	const bool				newlinesMakeNewRows	(ccGenConfig.fNewLinesAreNewRows);	//	Newline characters cause row breaks?
	const AtEndAction		endAction			(ccGenConfig.fEndAction);			//	What to do after last file is done playing
	NTV2StringList			filesToPlay			(ccGenConfig.fFilesToPlay);			//	List of text files to play
	UWord					linesWanted			(3);								//	Desired number of lines to Enqueue in one shot (min 1, max 4)
	bool					quitThisGenerator	(false);							//	Set true to exit this function/thread
	const UWord				paintPopTopRow		(9);								//	PaintOn/PopOn only:	top display row
	const UWord				paintPopMaxNumRows	(15 - paintPopTopRow + 1);			//	PaintOn/PopOn only:	number of rows to fill to bottom
	UWord					lineTally			(0);								//	PaintOn/PopOn only:	used to calculate display row
	const string			ccChannelStr		(::NTV2Line21ChannelToStr(inCCChannel));

	static const NTV2Line21Attrs sBlkYelSemi	(NTV2_CC608_Black,	NTV2_CC608_Yellow,	NTV2_CC608_SemiTransparent);
	static const NTV2Line21Attrs sBluCynItal	(NTV2_CC608_Blue,	NTV2_CC608_Cyan,	NTV2_CC608_Opaque,			/*italic*/true);
	static const NTV2Line21Attrs sRedBlkFlas	(NTV2_CC608_Red,	NTV2_CC608_Black,	NTV2_CC608_Opaque,			/*italic*/false,	/*UL*/false,	/*flash*/true);
	static const NTV2Line21Attrs sBluGrnSemiUL	(NTV2_CC608_Blue,	NTV2_CC608_Green,	NTV2_CC608_SemiTransparent, /*italic*/false,	/*UL*/true);
	static const NTV2Line21Attrs sMgnCynItal	(NTV2_CC608_Magenta,NTV2_CC608_Cyan,	NTV2_CC608_SemiTransparent,	/*italic*/true);
	static const NTV2Line21Attrs sAttrs[]	= {	NTV2Line21Attrs(), sBlkYelSemi, sBluCynItal, sRedBlkFlas, sBluGrnSemiUL, sMgnCynItal};

	PLNOTE("Started " << ccChannelStr << " generator thread");
	if (IsLine21TextChannel(inCCChannel)  ||  !IsLine21RollUpMode(captionMode))
		linesWanted = 1;
	while (!mCaptionGeneratorQuit)
	{
		CaptionSourceList	captionSources(::GetCaptionSources(filesToPlay, charsPerMinute));
		while (!mCaptionGeneratorQuit  &&  !captionSources.empty())
		{
			CaptionSourcePtr captionSource(captionSources.front());
			captionSources.pop_front();

			//	Set CaptionSource to Text Mode if caption channel is TX1/TX2/TX3/TX4...
			captionSource->SetTextMode(IsLine21TextChannel(inCCChannel));
			captionSource->SetCaptionChannel(inCCChannel);

			while (!mCaptionGeneratorQuit  &&  !captionSource->IsFinished())
			{
				//	Enqueue another message only if the encoder has less than 200 messages queued up...
				//	(This prevents runaway memory consumption)
				if (m608Encoder->GetQueuedMessageCount() < 200)
				{
					if (captionSource->IsPlainTextSource())
					{
						string	str	(captionSource->GetNextCaptionRow(newlinesMakeNewRows));

						if (IsLine21CaptionChannel(inCCChannel)  &&  !IsLine21RollUpMode(captionMode)  &&  linesWanted > 1)
							for (UWord lines(linesWanted - 1);  lines;  lines--)
								str += "\n" + captionSource->GetNextCaptionRow();

						if (!mConfig.fEmitStats && !str.empty())
							cout << str << endl;	//	Echo caption lines (if not emitting stats)
						PLDBG(ccChannelStr << " caption line " << DEC(lineTally+1) << ": '" << str << "'");

						const NTV2Line21Attrs & attrs (sAttrs[lineTally % 6]);	//	Cycle thru different display attributes

						//	For now, only the 608 encoder generates caption data.
						//	Someday we may generate captions using 708-specific features (e.g., windowing, etc.).

						//	Convert the UTF-8 string into a string containing CEA-608 byte codes expected
						//	by Enqueue*Message functions (except EnqueueTextMessage, which accepts UTF-8)...
						if (!IsLine21TextChannel(inCCChannel))
							str = CUtf8Helpers::Utf8ToCEA608String(str, inCCChannel);
						switch (captionMode)
						{
							case NTV2_CC608_CapModePopOn:
								m608Encoder->EnqueuePopOnMessage (str, inCCChannel,
																	/*row*/lineTally % paintPopMaxNumRows + paintPopTopRow,
																	/*col*/1,
																	/*attrs*/attrs);
								break;
							case NTV2_CC608_CapModeRollUp2:
							case NTV2_CC608_CapModeRollUp3:
							case NTV2_CC608_CapModeRollUp4:
								m608Encoder->EnqueueRollUpMessage (str, captionMode, inCCChannel,
																	/*row*/0, /*col*/0,
																	/*attrs*/attrs);
								break;
							case NTV2_CC608_CapModePaintOn:
								m608Encoder->EnqueuePaintOnMessage (str,
																	/*eraseFirst*/lineTally % paintPopMaxNumRows == 0,
																	/*chan*/inCCChannel,
																	/*row*/lineTally % paintPopMaxNumRows + paintPopTopRow,
																	/*col*/1,
																	/*attrs*/attrs);
								break;
							default:
								NTV2_ASSERT (IsLine21TextChannel(inCCChannel));
								m608Encoder->EnqueueTextMessage (str, lineTally == 0, inCCChannel);
								break;
						}	//	switch on caption mode
						lineTally++;
					}	//	if plaintext caption source
					else
					{	//	This caption source returns raw CC data byte pairs...
						SCCSource & sccSource(AsSCCSource(*captionSource));
						sccSource.EnqueueCCDataToFrame(m608Encoder, mACStatus.GetProcessedFrameCount());
					}
				}	//	if encoder has < 200 messages queued
				else
					AJATime::Sleep(1000);
			}	//	loop til captionSource is finished (or mCaptionGeneratorQuit)
			captionSource->SetCaptionChannel(NTV2_CC608_ChannelInvalid);
		}	//	for each captionSource

		switch (endAction)
		{
			case AtEndAction_Quit:		PLINFO(ccChannelStr << " generator signaling 'main' to terminate");
										filesToPlay.clear();			//	Clear to-do list if we loop again
										::SignalHandler(SIG_AJA_STOP);	//	Signal 'main' to terminate
										quitThisGenerator = true;		//	Instantly terminates me
										break;

			case AtEndAction_Repeat:	PLINFO(ccChannelStr << " generator repeating");
										break;

			case AtEndAction_Idle:		PLINFO(ccChannelStr << " generator entering idle mode");
										filesToPlay.clear();			//	Clear to-do list if we loop again
										quitThisGenerator = true;		//	Instantly terminates me
										break;

			default:					NTV2_ASSERT (false && "bad end action");
										break;
		}
		if (quitThisGenerator)
			break;
	}	//	loop til mCaptionGeneratorQuit

	//	Let's be nice, and inject an EDM (Erase Displayed Memory) control message.
	//	This will prevent frozen, on-screen captions from remaining in/on downstream decoders/monitors...
	m608Encoder->Erase (inCCChannel);
	PLNOTE(ccChannelStr << " generator thread exit");

}	//	GenerateCaptions


//////////////////////////////////////////////
//	Playout thread
//////////////////////////////////////////////


void NTV2CCPlayer::StartPlayoutThread ()
{
	//	Create and start the playout thread...
	mPlayThread.Attach(PlayThreadStatic, this);
	mPlayThread.SetPriority(AJA_ThreadPriority_High);
	mPlayThread.Start();
}	//	StartPlayoutThread


//	The playout thread function
void NTV2CCPlayer::PlayThreadStatic (AJAThread * pThread, void * pContext)		//	static
{	(void) pThread;
	//	Grab the NTV2CCPlayer instance pointer from the pContext parameter,
	//	then call its PlayoutFrames method...
	NTV2CCPlayer *	pApp (reinterpret_cast<NTV2CCPlayer*>(pContext));
	pApp->PlayoutFrames ();
}	//	PlayThreadStatic


void NTV2CCPlayer::PlayoutFrames (void)
{
	static const NTV2Line21Attributes		kBlueOnWhite	(NTV2_CC608_Blue,  NTV2_CC608_White,   NTV2_CC608_Opaque);
	static const NTV2Line21Attributes		kRedOnYellow	(NTV2_CC608_Red,   NTV2_CC608_Yellow,  NTV2_CC608_Opaque);
	static const uint32_t		AUDIOBYTES_MAX_48K	(201 * 1024);	//	Max audio bytes per frame (16 chls x 4 bytes x 67 msec/fr x 48000 Hz)
	static const double			gAmplitudes [16]	= {	0.10,			0.15,			0.20,			0.25,			0.30,			0.35,			0.40,
														0.45,			0.50,			0.55,			0.60,			0.65,			0.70,			0.75,
														0.80,			0.85};
	static const double			gFrequencies [16]	= {	150.00000000,	200.00000000,	266.66666667,	355.55555556,	474.07407407,	632.09876543,
														842.79835391,	1123.73113855,	1498.30818473,	1997.74424630,	2663.65899507,	3551.54532676,
														4735.39376902,	6313.85835869,	8418.47781159,	11224.63708211};
	//														1080i	720p	525i	625i	1080p	2KFilm	2K1080p	2K1080i	UHD	4K	UHDHFR	4KHFR
	static const uint16_t		gF2LineNums608[]	=	{	573,	0,		273,	323,	0,		1000,	0,		573,	0,		0,		0	};	//	Line 10 equivs for F2
	static const uint16_t		kF1PktLineNumCEA708(9), kF1PktLineNumCEA608(10);
	const NTV2SmpteLineNumber	smpteLineNumInfo	(::GetSmpteLineNumber(mVideoStandard));
	const uint16_t				kF2PktLineNumCEA608	(gF2LineNums608[mVideoStandard]);
	const uint32_t				F2StartLine			(smpteLineNumInfo.GetLastLine(smpteLineNumInfo.firstFieldTop ? NTV2_FIELD0 : NTV2_FIELD1) + 1);	//	F2 VBI starts here
	static const AJAAncDataLoc	kCEA708LocF1		(AJAAncDataLink_A,  AJAAncDataChannel_Y,  AJAAncDataSpace_VANC,  kF1PktLineNumCEA708, AJAAncDataHorizOffset_AnyVanc);
	const NTV2FormatDescriptor	formatDesc			(mConfig.fVideoFormat, mConfig.fPixelFormat, mConfig.fVancMode);
	const ULWord				bytesPerRow			(formatDesc.GetBytesPerRow());
	const bool					isProgressive		(::IsProgressivePicture(mConfig.fVideoFormat));
	const bool					isInterlaced		(!isProgressive);
	CNTV2Line21Captioner		Line21Encoder;		//	Used to encode "analog" (line 21) waveform
	CaptionData					captionData;		//	Current frame's 608 caption byte pairs (both fields)
	ULWord						acOptionFlags		(0);
	ULWord						currentSample		(0);
	NTV2AudioSystem				audioSystem			(NTV2_AUDIOSYSTEM_INVALID);
	ULWord						numAudioChannels	(0);
	Bouncer<UWord>				colBouncer			(32 - 11 /*upperLimit*/, 0 /*lowerLimit*/, 0 /*startAt*/);
	NTV2Buffer					audioBuffer;
	AUTOCIRCULATE_TRANSFER		xferInfo;

    ULWord ANCKB (2);	//	2 (default, works),  64 fails, 63 works										//	**MrBill**
	if (NTV2_IS_VANCMODE_OFF(mConfig.fVancMode))
	{
		mDevice.WriteRegister(10006, ANCKB);															//	**MrBill**
		PLINFO("Anc buffer size is " << DEC(ANCKB) << "K");												//	**MrBill**
		xferInfo.acANCBuffer.Allocate(ANCKB*1024);
		if (isInterlaced)
			xferInfo.acANCField2Buffer.Allocate(ANCKB*1024);
	}

	if (!mConfig.fSuppressAudio)
	{
		//	Audio setup...
		audioSystem = (mDevice.features().GetNumAudioSystems() > 1)  ?  ::NTV2ChannelToAudioSystem(mConfig.fOutputChannel)  :  NTV2_AUDIOSYSTEM_1;
		numAudioChannels = mDevice.features().GetMaxAudioChannels();
		if (NTV2_IS_4K_4096_VIDEO_FORMAT(mConfig.fVideoFormat)  &&  numAudioChannels > 8)
			numAudioChannels = 8;	//	2K/4096-pixel lines have narrower HANC space & can't handle 16 channels
		mDevice.SetNumberAudioChannels (numAudioChannels, audioSystem);		//	Config audio: # channels
		mDevice.SetAudioRate (NTV2_AUDIO_48K, audioSystem);					//	Config audio: 48kHz sample rate
		mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, audioSystem);	//	Config audio: Use 4MB output buffer
		mDevice.SetSDIOutputAudioSystem (mConfig.fOutputChannel, audioSystem);		//	Set output DS1 audio embedders to use designated audio system
		mDevice.SetSDIOutputDS2AudioSystem (mConfig.fOutputChannel, audioSystem);	//	Set output DS2 audio embedders to use designated audio system
		mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, audioSystem);	//	Config audio: Disable loopback (not E-E)
		audioBuffer.Allocate(AUDIOBYTES_MAX_48K);							//	Allocate audio buffer (large enough for one frame's audio)
		if (!mConfig.fDoMultiFormat)
		{
			for (UWord chan (0);  chan < mDevice.features().GetNumVideoOutputs();  chan++)
			{
				mDevice.SetSDIOutputAudioSystem (NTV2Channel (chan), audioSystem);
				mDevice.SetSDIOutputDS2AudioSystem (NTV2Channel (chan), audioSystem);
			}
			if (mDevice.features().GetNumHDMIAudioOutputChannels() == 8)
			{
				mDevice.SetHDMIOutAudioChannels (NTV2_HDMIAudio8Channels);
				mDevice.SetHDMIOutAudioSource8Channel (NTV2_AudioChannel1_8, audioSystem);
			}
		}	//	if not multiformat
	}	//	if audio not suppressed

	//	Set up the transfer buffers...
	xferInfo.SetVideoBuffer(reinterpret_cast<ULWord *>(mVideoBuffer.GetHostPointer()), mVideoBuffer.GetByteCount());
	if (mConfig.fSuppressTimecode)
		xferInfo.acOutputTimeCodes.Set (AJA_NULL, 0);
	PLNOTE("Playout thread started: 608F1PktLine=" << DEC(kF1PktLineNumCEA608) << " 608F2PktLine=" << DEC(kF2PktLineNumCEA608)
			<< " 708F1PktLine=" << DEC(kF1PktLineNumCEA708) << " F2StartLine=" << DEC(F2StartLine));

	NTV2FrameRate	frameRate(NTV2_FRAMERATE_UNKNOWN);
	mDevice.GetFrameRate(frameRate, mConfig.fOutputChannel);
	const TimecodeFormat tcFormat(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(frameRate));

	//	Set up playout AutoCirculate and start it...
	if (NTV2_IS_VANCMODE_OFF(mConfig.fVancMode))
		acOptionFlags |= AUTOCIRCULATE_WITH_ANC;
	if (!mConfig.fSuppressTimecode)
		acOptionFlags |= AUTOCIRCULATE_WITH_RP188;
	if (!mConfig.fSuppressTimecode  &&  !mConfig.fDoMultiFormat  &&  mDevice.features().GetNumLTCOutputs())
		acOptionFlags |= AUTOCIRCULATE_WITH_LTC;		//	Emit analog LTC if we "own" the device
	mDevice.AutoCirculateStop (mConfig.fOutputChannel);	//	Maybe some other app left this A/C channel running
	if (NTV2_IS_SD_VIDEO_FORMAT(mConfig.fVideoFormat)  &&  mConfig.fSuppressLine21  &&  mConfig.fSuppress608)
		cerr << "## WARNING:  SD video with '--noline21' option and '--no608' option won't produce captions" << endl;
	if (mDevice.AutoCirculateInitForOutput (mConfig.fOutputChannel,
											mConfig.fFrames.count(),	//	numFrames (zero if specifying range)
											audioSystem,
											acOptionFlags,
											1,							//	numChannels to gang
											mConfig.fFrames.firstFrame(), mConfig.fFrames.lastFrame()))
		mDevice.AutoCirculateStart (mConfig.fOutputChannel);
	else
		{cerr << "## ERROR: AutoCirculateInitForOutput failed" << endl;  mPlayerQuit = true;}

	//	Repeat until time to quit...
	while (!mPlayerQuit)
	{	//	Check AutoCirculate status...
		mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, mACStatus);
		if (!mACStatus.CanAcceptMoreOutputFrames())
		{	//	Out of room on the device for new frame...
			mDevice.WaitForOutputVerticalInterrupt (mConfig.fOutputChannel);	//	Wait for next VBI
			continue;	//	Try again
		}
		if (NTV2_IS_VANCMODE_OFF(mConfig.fVancMode))
			{xferInfo.acANCBuffer.Fill(ULWord(0));	xferInfo.acANCField2Buffer.Fill(ULWord(0));}	//	Clear Anc buffers before filling

		AJAAncillaryList	packetList;	//	List of packets to be transmitted
		m608Encoder->GetNextCaptionData(captionData);	//	Pop queued captions from 608 encoder waiting to be transmitted
		if (!mConfig.fSuppress608)
		{
			AJAAncillaryData_Cea608_Vanc	pkt608F1;

			pkt608F1.SetLocationLineNumber (kF1PktLineNumCEA608);
			pkt608F1.SetCEA608Bytes (captionData.f1_char1, captionData.f1_char2);
			pkt608F1.GeneratePayloadData();
			packetList.AddAncillaryData(pkt608F1);
			if (!NTV2_IS_PROGRESSIVE_STANDARD(mVideoStandard))
			{	NTV2_ASSERT(kF2PktLineNumCEA608);
				AJAAncillaryData_Cea608_Vanc	pkt608F2;
				pkt608F2.SetLocationLineNumber(kF2PktLineNumCEA608);
				pkt608F2.SetCEA608Bytes (captionData.f2_char1, captionData.f2_char2);
				pkt608F2.GeneratePayloadData();
				packetList.AddAncillaryData(pkt608F2);
			}
		}

		if (NTV2_IS_SD_VIDEO_FORMAT(mConfig.fVideoFormat))
		{	//	SD Video encodes "analog" waveform into line 21...
			if (!mConfig.fSuppressLine21)
			{	//	Overwrite Line21 with encoded CEA608 waveform
				ULWord	line21RowOffset		(0);
				UByte *	pLine21				(AJA_NULL);
				UByte *	pEncodedYUV8Line	(AJA_NULL);
				formatDesc.GetLineOffsetFromSMPTELine (21, line21RowOffset);
				pLine21 = reinterpret_cast<UByte*>(formatDesc.GetWriteableRowAddress(mVideoBuffer.GetHostPointer(),  line21RowOffset));
				if (pLine21)
				{	//	Encode F1 caption bytes into EIA-608-compliant 8-bit YUV waveform...
					pEncodedYUV8Line = Line21Encoder.EncodeLine (captionData.f1_char1, captionData.f1_char2);
					//	Replace F1 line 21 in the frame buffer with the EncodeLine result...
					if (mConfig.fPixelFormat == NTV2_FBF_8BIT_YCBCR)
						::memcpy (pLine21, pEncodedYUV8Line, bytesPerRow);	//	... just copy the line
					else if (mConfig.fPixelFormat == NTV2_FBF_10BIT_YCBCR)
						::ConvertLine_2vuy_to_v210 (pEncodedYUV8Line, reinterpret_cast<ULWord*>(pLine21), 720);	//	...with EncodeLine result converted to 10-bit YUV
				}
				else PLFAIL("GetWriteableRowAddress return NULL for SMPTE line 21, rowOffset=" << xHEX0N(line21RowOffset,8) << " " << formatDesc);

				formatDesc.GetLineOffsetFromSMPTELine (284, line21RowOffset);
				pLine21 = reinterpret_cast<UByte*>(formatDesc.GetWriteableRowAddress(mVideoBuffer.GetHostPointer(),  line21RowOffset));
				if (pLine21)
				{	//	Encode F2 caption bytes into EIA-608-compliant 8-bit YUV waveform...
					pEncodedYUV8Line = Line21Encoder.EncodeLine (captionData.f2_char1, captionData.f2_char2);
					//	Replace F2 line 21 in the frame buffer with the EncodeLine result...
					if (mConfig.fPixelFormat == NTV2_FBF_8BIT_YCBCR)
						::memcpy (pLine21, pEncodedYUV8Line, bytesPerRow);	//	... just copy the line
					else if (mConfig.fPixelFormat == NTV2_FBF_10BIT_YCBCR)
						::ConvertLine_2vuy_to_v210 (pEncodedYUV8Line, reinterpret_cast<ULWord*>(pLine21), 720);	//	...with EncodeLine result converted to 10-bit YUV
				}
				else PLFAIL("GetWriteableRowAddress return NULL for SMPTE line 284, rowOffset=" << xHEX0N(line21RowOffset,8) << " " << formatDesc);
				//DEBUG		if (captionData.HasData())	mDevice.DMAWriteFrame(33, (ULWord*) mVideoBuffer.GetHostPointer(), mVideoBuffer.GetByteCount());
			}	//	if not suppressing analog line21
		}	//	if SD video
		else if (!mConfig.fSuppress708)
		{	//	HD video -- use the 708 encoder to put 608 captions into a single 708 packet:
			m708Encoder->Set608CaptionData(captionData);	//	Set the 708 encoder's 608 caption data (for both F1 and F2)
			if (m708Encoder->MakeSMPTE334AncPacket (frameRate, NTV2_CC608_Field1))		//	Generate SMPTE-334 Anc data packet
			{
				AJAAncillaryData_Cea708	pkt708;
				pkt708.SetFromSMPTE334 (m708Encoder->GetSMPTE334Data(), uint32_t(m708Encoder->GetSMPTE334Size()), kCEA708LocF1);
				packetList.AddAncillaryData(pkt708);
			}
		}	//	else HD video

		//	PLDBG("Xmit pkts: " << packetList);	//	DEBUG: Packet list to be transmitted
		packetList.SetAllowMultiRTPTransmit(mConfig.fForceRTP & BIT(1));
		if (NTV2_IS_VANCMODE_ON(mConfig.fVancMode))	//	Write FB VANC lines...
			packetList.GetVANCTransmitData (mVideoBuffer,  formatDesc);
		else if (mConfig.fForceRTP)	//	Force RTP? Non-IP2110 devices won't understand what's in the Anc buffer...
			packetList.GetIPTransmitData(xferInfo.acANCBuffer, xferInfo.acANCField2Buffer, isProgressive, F2StartLine);
		else	//	Else use the Anc inserter firmware:
			packetList.GetTransmitData (xferInfo.acANCBuffer, xferInfo.acANCField2Buffer, isProgressive, F2StartLine);

		if (mConfig.fForceRTP  &&  mConfig.fForceRTP & BIT(1)  &&  mDevice.features().CanDo2110())
			xferInfo.acTransferStatus.acState = NTV2_AUTOCIRCULATE_INVALID;	//	Signal MultiPkt RTP to AutoCirculateTransfer/S2110DeviceAncToXferBuffers

		if (!mConfig.fSuppressTimecode)
		{
			const CRP188	rp188		(mACStatus.GetProcessedFrameCount(), tcFormat);
			char			tcString[]	= {"                                "};
			const UWord		colShift	(mACStatus.GetProcessedFrameCount() % 10 == 0  ?  colBouncer.Next()  :  colBouncer.Value());
			bool			tcOK		(false);
			NTV2_RP188		tc;
			rp188.GetRP188Reg  (tc);
			if (!NTV2_IS_QUAD_FRAME_FORMAT(mConfig.fVideoFormat) && !mConfig.fDoMultiFormat)
				//	UniFormat and not Quad Frame:   i.e. using ALL output spigots:
				//	AutoCirculateTransfer will automatically set ALL output timecodes to
				//	the DEFAULT timecode if the DEFAULT timecode is valid...
				tcOK = xferInfo.SetOutputTimeCode(tc, NTV2_TCINDEX_DEFAULT);
			else
			{	//	MULTI-FORMAT OR QUAD-FRAME:
				//	Be more selective as to which output spigots get the generated timecode...
				NTV2TimeCodes	timecodes;
				for (int num (0);  num < 4;  num++)
				{
					const NTV2Channel	chan (NTV2Channel(mConfig.fOutputChannel + num));
					timecodes[::NTV2ChannelToTimecodeIndex(chan, /*inEmbeddedLTC*/false)] = tc;
					timecodes[::NTV2ChannelToTimecodeIndex(chan, /*inEmbeddedLTC*/true)] = tc;
					if (isInterlaced)
						timecodes[::NTV2ChannelToTimecodeIndex(chan, /*inEmbeddedLTC*/false, /*inIsF2*/true)] = tc;
					if (acOptionFlags & AUTOCIRCULATE_WITH_LTC)
					{
						timecodes[NTV2_TCINDEX_LTC1] = tc;
						if (mDevice.features().GetNumLTCOutputs() > 1)
							timecodes[NTV2_TCINDEX_LTC2] = tc;
					}
					if (!NTV2_IS_QUAD_FRAME_FORMAT(mConfig.fVideoFormat))
						break;	//	Not Quad Frame:  just do the one output
				}	//	for each quad
				tcOK = xferInfo.SetOutputTimeCodes(timecodes);
			}
			::memcpy (tcString + colShift, rp188.GetRP188CString(), 11);
			CNTV2CaptionRenderer::BurnString (tcString, tcOK ? kBlueOnWhite : kRedOnYellow, mVideoBuffer, formatDesc, 3, 1);	//	R3C1
		}	//	if not suppressing timecode injection

		if (audioBuffer)
			xferInfo.SetAudioBuffer (audioBuffer,
									::AddAudioTone (audioBuffer,		//	audio buffer to fill
													currentSample,		//	sample for continuing the waveform
													::GetAudioSamplesPerFrame(frameRate, NTV2_AUDIO_48K, mACStatus.GetProcessedFrameCount()),	//	# samples to generate
													48000.0,			//	sample rate [Hz]
													gAmplitudes,		//	per-channel amplitudes
													gFrequencies,		//	per-channel tone frequencies [Hz]
													31,					//	bits per sample
													false,				//	false means "don't byte swap"
													numAudioChannels));	//	number of audio channels
		//	Finally ... transfer the frame data to the device...
		if (!mDevice.AutoCirculateTransfer (mConfig.fOutputChannel, xferInfo))
			PLFAIL("AutoCirculateTransfer failed");

		ULWord val(0);																					//	**MrBill**
		if (xferInfo.acANCBuffer  &&  mDevice.ReadRegister(10006, val)  &&  val  &&  val != ANCKB)		//	**MrBill**
		{																								//	**MrBill**
			PLINFO("Anc buffer size changed from " << DEC(ANCKB) << "K to " << DEC(val) << "K");		//	**MrBill**
			ANCKB = val;																				//	**MrBill**
			xferInfo.acANCBuffer.Allocate(ANCKB*1024);													//	**MrBill**
			if (isInterlaced)																			//	**MrBill**
				xferInfo.acANCField2Buffer.Allocate(ANCKB*1024);										//	**MrBill**
		}																								//	**MrBill**
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop (mConfig.fOutputChannel);

	//	Flush encoder queues (prevent Quit from hanging)...
	while(m608Encoder->GetQueuedByteCount(NTV2_CC608_Field1) || m608Encoder->GetQueuedByteCount(NTV2_CC608_Field2))
		{m608Encoder->Flush(NTV2_CC608_Field1);  m608Encoder->Flush(NTV2_CC608_Field2);}
	PLNOTE("Playout thread exit");

}	//	PlayoutFrames


void NTV2CCPlayer::GetStatus (	size_t & outMessagesQueued,	size_t & outBytesQueued,
								size_t & outTotMsgsEnq,		size_t & outTotBytesEnq,
								size_t & outTotMsgsDeq,		size_t & outTotBytesDeq,
								size_t & outMaxQueDepth,	size_t & outDroppedFrames) const
{
	mDevice.WaitForOutputVerticalInterrupt (NTV2_CHANNEL1, 60);
	if (m608Encoder)
	{
		outMessagesQueued	= m608Encoder->GetQueuedMessageCount();
		outBytesQueued		= m608Encoder->GetQueuedByteCount();
		outTotMsgsEnq		= m608Encoder->GetEnqueueMessageTally();
		outTotBytesEnq		= m608Encoder->GetEnqueueByteTally();
		outTotMsgsDeq		= m608Encoder->GetDequeueMessageTally();
		outTotBytesDeq		= m608Encoder->GetDequeueByteTally();
		outMaxQueDepth		= m608Encoder->GetHighestQueueDepth();
	}
	else
		outMessagesQueued = outBytesQueued = outTotMsgsEnq = outTotBytesEnq = outTotMsgsDeq = outTotBytesDeq = outMaxQueDepth = 0;
	outDroppedFrames = size_t(mACStatus.GetDroppedFrameCount());

}	//	GetStatus
