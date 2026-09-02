/**
	@file		ntv2srt.cpp
	@brief		Implementation of SubRipText codec classes.
	@copyright	(C) 2021-2022 AJA Video Systems, Inc. All rights reserved.
**/

#include "ntv2srt.h"
#include "ajabase/common/common.h"
#include "ajabase/system/info.h"
#include "ntv2publicinterface.h"
#include <sstream>

using namespace std;

typedef SRTCaptions::CaptionMap::iterator			CaptionMapIter;
typedef SRTCaptions::CaptionMap::const_iterator		CaptionMapConstIter;
typedef SRTCaptions::DurationMap::iterator			DurationMapIter;
typedef SRTCaptions::DurationMap::const_iterator	DurationMapConstIter;
typedef SRTCaptions::TimestampMap::iterator			TimestampMapIter;
typedef SRTCaptions::TimestampMap::const_iterator	TimestampMapConstIter;
typedef SRTCaptions::SeqNumMap::iterator			SeqNumMapIter;
typedef SRTCaptions::SeqNumMap::const_iterator		SeqNumMapConstIter;
typedef SRTCaptions::WindowMap::iterator			WindowMapIter;
typedef SRTCaptions::WindowMap::const_iterator		WindowMapConstIter;
typedef SRTCaptions::AnchorMap::iterator			AnchorMapIter;
typedef SRTCaptions::AnchorMap::const_iterator		AnchorMapConstIter;
typedef SRTCaptions::ErrorMap::const_iterator		ErrorMapConstIter;
typedef SRTCaptions::SourceMap::const_iterator		SourceMapConstIter;

static const int kStateBad(0), kStateSeqNum(1), kStateTimestamps(2), kStateCaption(3);	//	Parse states
static const uint32_t kMSPerSecond(1000), kMSPerMinute(60*kMSPerSecond), kMSPerHour(60*kMSPerMinute), kMSPerDay(24*kMSPerHour), kMSPerWeek(7*kMSPerDay);
static const string gAnchorStrs[10] = {"<none>", "Bottom-Left", "Bottom-Center", "Bottom-Right", "Middle-Left", "Middle-Center", "Middle-Right", "Top-Left", "Top-Center", "Top-Right"};
static const string gAnchorTags[10] = {"", "{\\an1}", "{\\an2}", "{\\an3}", "{\\an4}", "{\\an5}", "{\\an6}", "{\\an7}", "{\\an8}", "{\\an9}"};

SRTDuration	SRTCaptions::gMinCaptionDuration(500);		//	Minimum acceptable duration:  500 msec default
const SRTCaptions::StringList SRTCaptions::kEmptyList;


bool SRTCaptions::getTimestamp (const string & inStr, SRTTimestamp & outTS, ostream & oss)
{
	string str(inStr);
	SRTCaptions::StringList halves(aja::split(str, ","));
	outTS = 0;
	if (halves.empty())
		{oss << "Empty timecode string";  return false;}
	if (halves.size() < 2)
		{oss << "Missing ',' in timecode '" << str << "'";  return false;}
	if (halves.size() > 2)
		{oss << "Multiple ',' in timecode '" << str << "'";  return false;}
	aja::strip(halves.at(0));
	aja::strip(halves.at(1));
	if (!aja::is_legal_decimal_number(halves.at(1), 3))
		{oss << "Bad millisecond value '" << halves.at(1) << "' in timecode '" << str << "', expected decimal integer";  return false;}
	uint32_t compMilliSec(aja::stoull(halves.at(1))), compSec(0), compMin(0), compHr(0);
	if (compMilliSec > 999)
		{oss << "Bad millisecond value '" << DEC(compMilliSec) << "' in timecode '" << str << "', expected value < 1000";  return false;}

	SRTCaptions::StringList comps(aja::split(halves.at(0),":"));
	if (comps.empty())
		{oss << "Empty HH:MM:SS timecode portion";  return false;}
	if (comps.size() < 3)
		{oss << "Missing ':' in HH:MM:SS timecode portion: '" << str << "'";  return false;}
	if (comps.size() > 3)
		{oss << "Multiple ',' in HH:MM:SS timecode portion: '" << str << "'";  return false;}

	aja::strip(comps.at(0));
	aja::strip(comps.at(1));
	aja::strip(comps.at(2));
	if (!aja::is_legal_decimal_number(comps.at(0)))
		{oss << "Bad hours value '" << comps.at(0) << "' in timecode '" << str << "', expected decimal integer";  return false;}
	if (!aja::is_legal_decimal_number(comps.at(1)))
		{oss << "Bad minutes value '" << comps.at(1) << "' in timecode '" << str << "', expected decimal integer";  return false;}
	if (!aja::is_legal_decimal_number(comps.at(2)))
		{oss << "Bad seconds value '" << comps.at(2) << "' in timecode '" << str << "', expected decimal integer";  return false;}

	compHr = aja::stoull(comps.at(0));
	compMin = aja::stoull(comps.at(1));
	compSec = aja::stoull(comps.at(2));
	if (compHr > 1176)
		{oss << "Bad hours value '" << DEC(compHr) << "' in timecode '" << str << "', expected value < 1176";  return false;}
	if (compMin > 59)
		{oss << "Bad minutes value '" << DEC(compMin) << "' in timecode '" << str << "', expected value < 60";  return false;}
	if (compSec > 59)
		{oss << "Bad seconds value '" << DEC(compSec) << "' in timecode '" << str << "', expected value < 60";  return false;}
	outTS = (compHr * 60 * 60  +  compMin * 60  +  compSec) * 1000  +  compMilliSec;
	return true;
}

typedef map<string, string>	StringMap;

bool SRTCaptions::getWindow (const string & inStr, SRTWindow & outWndo, ostream & oss)
{
	string str(inStr);
	aja::strip(str);
	outWndo.clear();
	if (str.empty())
		{oss << "Empty window string";  return false;}
	StringList chunks(aja::split(str, " ")), quads;
	for (size_t ndx(0);  ndx < chunks.size();  ndx++)
		if (!aja::strip(chunks.at(ndx)).empty())
			quads.push_back(chunks.at(ndx));
	if (quads.size() != 4)
		{oss << DEC(quads.size()) << " space-delimited coordinate spec(s) found, expected 4";  return false;}
	static const string legalKeys[] = {"X1", "X2", "Y1", "Y2", ""};
	StringMap keysVals;	//	e.g. "X1" => "123", "X2" => "234", etc.
	for (size_t ndx(0);  ndx < 4;  ndx++)
	{
		if (quads.at(ndx).find(":") == string::npos)
			{oss << "Window rect quadrant " << DEC(ndx+1) << " ('" << quads.at(ndx) << "') missing ':'";  return false;}
		StringList coordPair(aja::split(quads.at(ndx), ":"));
		if (coordPair.size() != 2)
			{oss << "Window rect quadrant " << DEC(ndx+1) << " ('" << quads.at(ndx) << "') has " << DEC(coordPair.size()) << " slice(s) from ':', expected 2";  return false;}
		aja::upper(coordPair.at(0));
		if (coordPair.at(0).length() != 2)
			{oss << "Window rect quadrant " << DEC(ndx+1) << " ('" << quads.at(ndx) << "') slice '" << coordPair.at(0) << "' length is " << DEC(coordPair.at(0).length()) << ", expected 2";  return false;}
		if (coordPair.at(0).at(0) != 'X'  &&  coordPair.at(0).at(0) != 'Y')
			{oss << "Window rect quadrant " << DEC(ndx+1) << " ('" << quads.at(ndx) << "') slice '" << coordPair.at(0) << "' starts with '" << coordPair.at(0).at(0) << "', expected 'X' or 'Y'";  return false;}
		if (keysVals.find(quads.at(ndx)) != keysVals.end())
			{oss << "Window rect coordinate " << coordPair.at(0) << "' specified more than once";  return false;}
		keysVals[coordPair.at(0)] = coordPair.at(1);
	}
	if (keysVals.size() < 4)
		{oss << "Incomplete window rect, missing " << DEC(4-keysVals.size()) << " coordinate(s)";  return false;}
	NTV2_ASSERT(keysVals.size() == 4);
	//	Check coordinate values
	SRTWindow wndo;
	for (size_t ndx(0);  ndx < 4;  ndx++)
	{
		string val(keysVals[legalKeys[ndx]]);
		if (!aja::is_legal_decimal_number(aja::strip(val), 4))
			{oss << "Window rect coordinate '" << legalKeys[ndx] << "' is '" << val << "', expected decimal integer";  return false;}
		switch(ndx)
		{
			case 0:	/* X1 */	wndo.setLeft(aja::stoull(val));		break;
			case 1:	/* X2 */	wndo.setRight(aja::stoull(val));	break;
			case 2:	/* Y1 */	wndo.setTop(aja::stoull(val));		break;
			case 3:	/* Y2 */	wndo.setBottom(aja::stoull(val));	break;
			default:	NTV2_ASSERT(false);
		}
	}
	if (!wndo.isValid())
		{oss << "Invalid window X1=" << DEC(wndo.left()) << " Y1=" << DEC(wndo.top()) << " X2=" << DEC(wndo.right()) << " Y2=" << DEC(wndo.bottom());  return false;}
	if (wndo.isEmpty())
		{oss << "Empty window X1=" << DEC(wndo.left()) << " Y1=" << DEC(wndo.top()) << " X2=" << DEC(wndo.right()) << " Y2=" << DEC(wndo.bottom());  return false;}
	outWndo = wndo;
	return true;
}

string SRTCaptions::timestampStr (const SRTTimestamp inTS, const bool inIncludeRawValue)
{
	ostringstream oss;
	uint32_t msec(inTS), hrs(0), mins(0), secs(0);
	hrs =  msec / kMSPerHour;
	msec -= hrs * kMSPerHour;
	mins = msec / kMSPerMinute;
	msec -= mins * kMSPerMinute;
	secs = msec / kMSPerSecond;
	msec -= secs * kMSPerSecond;
	oss << DEC0N(hrs,2) << ":" << DEC0N(mins,2) << ":" << DEC0N(secs,2) << "," << DEC0N(msec,3);
	if (inIncludeRawValue)
		oss << " (" << DEC(inTS) << ")";
	return oss.str();
}

string SRTCaptions::durationStr (const SRTDuration inDurationMS, const bool inIncludeRawValue)
{
	SRTDuration		durationMS(inDurationMS);
	const uint32_t	weeks	(durationMS / kMSPerWeek);		durationMS -= weeks * kMSPerWeek;
	const uint32_t	days	(durationMS / kMSPerDay);		durationMS -= days * kMSPerDay;
	const uint32_t	hours	(durationMS / kMSPerHour);		durationMS -= hours * kMSPerHour;
	const uint32_t	mins	(durationMS / kMSPerMinute);	durationMS -= mins * kMSPerMinute;
	const uint32_t	secs	(durationMS / kMSPerSecond);	durationMS -= secs * kMSPerSecond;
	ostringstream oss;
	if (weeks)
		oss << DEC(weeks) << " week" << (weeks == 1 ? "" : "s");
	if (days  &&  oss.str().length())
		oss << " ";
	if (days)
		oss << DEC(days) << " day" << (days == 1 ? "" : "s");
	if (hours  &&  oss.str().length())
		oss << " ";
	if (hours)
		oss << DEC(hours) << " hour" << (hours == 1 ? "" : "s");
	if (mins  &&  oss.str().length())
		oss << " ";
	if (mins)
		oss << DEC(mins) << " min" << (mins == 1 ? "" : "s");
	if (secs  &&  oss.str().length())
		oss << " ";
	if (secs)
		oss << DEC(secs) << "." << DEC0N(durationMS,3) << " sec" << (secs < 2 ? "" : "s");
	else if (oss.str().empty())
		oss << DEC(durationMS) << " msec";
	if (inIncludeRawValue)
		oss << (oss.str().empty() ? "" : " ") << "(" << DEC(inDurationMS) << ")";
	return oss.str();
}

string SRTCaptions::anchorStr (const uint16_t inAnchor, const bool inIncludeRawValue)
{
	ostringstream oss;
	uint16_t anchor(inAnchor < 10 ? inAnchor : 0);
	oss << gAnchorStrs[anchor];
	if (inIncludeRawValue)
		oss << " (" << DEC(inAnchor) << ")";
	return oss.str();
}

string SRTCaptions::anchorTag (const uint16_t inAnchor)
{
	if (inAnchor < 10)
		return gAnchorTags[inAnchor];
	return "<invalid>";
}

string SRTCaptionInfo::caption (void) const
{
	static const SRTCaptions::StringList sTagsToRmv = {	gAnchorTags[1], gAnchorTags[2], gAnchorTags[3], gAnchorTags[4],
														gAnchorTags[5], gAnchorTags[6], gAnchorTags[7], gAnchorTags[8],
														gAnchorTags[9], "<b>", "</b>", "<i>", "</i>", "<u>", "</u>"};
	string result(rawCaption());
	//	Remove all simple tags...
	for (size_t ndx(0);  ndx < sTagsToRmv.size();  ndx++)
		if (result.find(sTagsToRmv.at(ndx)) != string::npos)
			aja::replace(result, sTagsToRmv.at(ndx), "");
	//	TBD:  Remove font color tags...
	return result;
}

bool SRTCaptionInfo::getCharWithAttrs (const size_t inPos, string & outChar, bool & outIsBold, bool & outIsItalic, string & outColor) const
{
	outChar.clear();
	outColor.clear();
	outIsBold = outIsItalic = false;
	const string captionText(caption());
	if (inPos >= captionText.length())
		return false;
	outChar += captionText.at(inPos);
	return true;
}

string SRTWindow::asString (void) const
{
	ostringstream oss;
	print(oss);
	return oss.str();
}

ostream & SRTWindow::print (ostream & oss) const
{
	if (isValid()  &&  !isEmpty())
		oss << DEC(width()) << "Wx" << DEC(height()) << "H at " << DEC(left()) << "X," << DEC(top()) << "Y";
	else
		oss << "<none>";
	return oss;
}

ostream & SRTCaptionInfo::printInfo (ostream & inOutStrm) const
{
	if (!isValid())
		return inOutStrm;
	AJALabelValuePairs info;
	{ostringstream oss;  oss << DEC(sequenceNum()); AJASystemInfo::append(info, "Sequence Number", oss.str());}
	{ostringstream oss;  oss << SRTCaptions::timestampStr(startTime(), true);	AJASystemInfo::append(info, "Start Time", oss.str());}
	{ostringstream oss;  oss << SRTCaptions::timestampStr(endTime(), true);	AJASystemInfo::append(info, "End Time", oss.str());}
	{ostringstream oss;  oss << SRTCaptions::durationStr(duration());	AJASystemInfo::append(info, "Duration", oss.str());}
	if (hasLineNumbers())
		{ostringstream oss;  oss << DEC(startLineNumber()) << " thru " << DEC(endLineNumber()); AJASystemInfo::append(info, "Line Numbers", oss.str());}
	{AJASystemInfo::append(info, "Raw Caption Text", rawCaption());}
	{AJASystemInfo::append(info, "Caption Text", caption());}
	{ostringstream oss;  oss << window();	AJASystemInfo::append(info, "Window", oss.str());}
	{ostringstream oss;  oss << SRTCaptions::anchorStr(anchor(),true);	AJASystemInfo::append(info, "Anchor", oss.str());}
	inOutStrm << AJASystemInfo::ToString(info);
	return inOutStrm;
}

ostream & SRTCaptionInfo::printSRT (ostream & oss) const
{
	if (!isValid())
		return oss;
	if (!isFirstCaption())
		oss << endl;	//	Prepend blank line if not first caption in a collection
	oss	<< DEC(sequenceNum()) << endl
		<< SRTCaptions::timestampStr(startTime()) << " --> " << SRTCaptions::timestampStr(endTime()) << endl
		<< rawCaption() << endl;
	return oss;
}


bool SRTCaptions::reloadFrom (const StringList & inSRTData)
{
	static vector<int> states = {kStateBad, kStateSeqNum, kStateTimestamps, kStateCaption};
	clear();
	int state (kStateSeqNum);
	size_t startSrcLine(0);
	SRTSeqNum lastSeqNum(0), seqNum(0);
	SRTTimestamp startTS(0), endTS(0);
	SRTDuration durationMS(0);
	SRTWindow window;
	uint16_t anchorPos(0);
	string captionText;
	mLines = inSRTData;
	while (mLinesParsed < totalLines())
	{
		size_t lineOffset(mLinesParsed++);	//	0-based offset (0, 1, 2, ...)
		size_t lineNumber(mLinesParsed);	//	1-based number (1, 2, 3, ...)
		string line(mLines.at(lineOffset));
		aja::strip(line);
		ostringstream err, tcErr;
		switch (state)
		{
			case kStateSeqNum:
				if (line.empty())
				{
					err << "Expected sequence number, instead got blank line";
					mWarnings[lineOffset] = err.str();
					continue;
				}
				if (!aja::is_legal_decimal_number(line, 8))	//	8 digits, up to 99,999,999
				{
					err << "Expected sequence number, instead got '" << line << "'";
					mErrors[lineOffset] = err.str();
					break;	//	Expected decimal number
				}
				seqNum = aja::stoull(line);
				if (!seqNum)
				{
					err << "Illegal sequence number, expected non-zero value, instead got '0'";
					mErrors[lineOffset] = err.str();
					break;	//	Expected non-zero value
				}
				if (seqNum <= lastSeqNum)
				{
					err << "Old sequence number reused, expected '" << DEC(lastSeqNum+1) << "', instead got '" << DEC(seqNum) << "'";
					mErrors[lineOffset] = err.str();
					break;	//	Past sequence number
				}
				if (seqNum > (lastSeqNum+1))
				{
					err << "Bad sequence number, expected '" << DEC(lastSeqNum+1) << "', instead got '" << DEC(seqNum) << "'";
					mWarnings[lineOffset] = err.str();
				}
				startSrcLine = lineNumber;	//	startSrcLine is 1-based (+1 from source line offset)
				state = kStateTimestamps;	//	Move to timestamp
				break;

			case kStateTimestamps:
			{
				StringList timestamps, tss;
				string wndoStr;
				if (line.empty())
				{
					err << "Expected timecode spec, instead got blank line";
					mWarnings[lineOffset] = err.str();
					continue;
				}
				if (line.find("-->") != string::npos)
					tss = aja::split(line, "-->");
				else if (line.find("->") != string::npos)
				{
					ostringstream tmp;
					tss = aja::split(line, "->");
					tmp << "Expected '-->' in timecode spec, instead got '->'";
					mWarnings[lineOffset] = tmp.str();
				}
				else
				{
					err << "Bad timecode spec, missing '-->': '" << line << "'";
					mErrors[lineOffset] = err.str();
					break;	//	Missing '-->' delimiter
				}

				for (size_t n(0);  n < tss.size();  n++)
				{	string ts(tss.at(n));
					if (!aja::strip(ts).empty())
						timestamps.push_back(ts);
				}

				if (timestamps.size() > 2)
				{
					err << DEC(timestamps.size()) << " timecodes specified, expected 2: '" << line << "'";
					mErrors[lineOffset] = err.str();
					break;	//	Future sequence number
				}
				NTV2_ASSERT(timestamps.size() == 2);
				if (!getTimestamp(timestamps.at(0), startTS, tcErr)  &&  !tcErr.str().empty())
				{
					err << "Bad starting time: " << tcErr.str();
					mErrors[lineOffset] = err.str();
					break;	//	Bad start timecode
				}
				SRTCaptionInfo info;
				if (captionAtStartTime(startTS, info))
				{
					err << "Start time " << timestampStr(startTS,true) << " duplicated from caption " << DEC(info.sequenceNum()) << tcErr.str();
					mErrors[lineOffset] = err.str();
					break;	//	Duplicate starttime
				}
				if (timestamps.at(1).length() > 12)
				{
					wndoStr = timestamps.at(1).substr(12,9999);	//	Extra stuff might be window spec
					timestamps.at(1).erase(12,9999);	//	Lop off extra stuff
				}
				if (!getTimestamp(timestamps.at(1), endTS, tcErr)  &&  !tcErr.str().empty())
				{
					err << "Bad ending time: " << tcErr.str();
					mErrors[lineOffset] = err.str();
					break;	//	Bad end timecode
				}
				if (startTS >= endTS)
				{
					err << "Start time " << timestampStr(startTS,true) << " happens after end time " << timestampStr(endTS,true);
					mErrors[lineOffset] = err.str();
					break;	//	Start timecode happens after end timecode
				}
				durationMS = endTS - startTS;
				if (durationMS < defaultMinimumDuration())
				{
					err << "Caption duration " << durationStr(durationMS,true) << " less than " << durationStr(defaultMinimumDuration());
					if (mWarnings[lineOffset].empty())
						mWarnings[lineOffset] = err.str();
					else
						mWarnings[lineOffset] += "\n          " + err.str();
				}
				if (!wndoStr.empty()  &&  !getWindow(wndoStr, window, err))
				{
					mErrors[lineOffset] = err.str();
					break;	//	Bad window spec
				}
				state = kStateCaption;	//	Move to caption lines
				break;
			}	//	case kStateTimestamps

			case kStateCaption:
				//	Check for anchor tag...
				for (uint16_t num(1);  num <= 9;  num++)
				{	const string anchStr(gAnchorTags[num]);
					const size_t anchPos(line.find(anchStr));
					if (anchPos == string::npos)
						continue;	//	Not found ... keep looking
					if (anchPos)
					{
						err << "Anchor tag '" << anchStr << "' at offset " << DEC(anchPos) << ", expected at start of line";
						mErrors[lineOffset] = err.str();
						break;
					}
					else if (!captionText.empty())
					{
						err << "Anchor tag '" << anchStr << "' must appear at start of caption text";
						mErrors[lineOffset] = err.str();
						break;
					}
					else if (anchorPos == num)
					{
						err << "Duplicate anchor tag '" << anchStr << "' specified";
						mErrors[lineOffset] = err.str();
						break;
					}
					else if (anchorPos)
					{
						err << "Multiple anchor tags specified: '" << anchStr << "', '" << gAnchorTags[anchorPos] << "' specified";
						mErrors[lineOffset] = err.str();
						break;
					}
					else if (window.isValid())
					{
						err << "Anchor tag '" << anchStr << "' conflicts with window spec '" << window.asString() << "'";
						mErrors[lineOffset] = err.str();
						break;
					}
					anchorPos = num;
					line.erase(0, anchStr.length());	//	Remove anchor tag...
					aja::strip(line);	//	...and any whitespace after the tag
				}	//	for each possible anchor tag
				if (!mErrors.empty())
					break;	//	Failed anchor tag check

				if (captionText.empty()  &&  line.empty())
				{
					mErrors[lineOffset] = "Expected caption text, instead got blank line";
					break;
				}
				else if (!captionText.empty()  &&  line.empty())
				{
					SRTCaptionInfo info (seqNum,  startTS,  durationMS,  captionText, startSrcLine, lineNumber-1);
					if (isEmpty())
						info.setFirstCaption();
					if (anchorPos)
						NTV2_ASSERT(anchorPos  && !window.isValid());
					if (window.isValid())
						NTV2_ASSERT(!anchorPos  && window.isValid());
					if (anchorPos)
						info.setAnchorPos(anchorPos);
					if (window.isValid())
						info.setWindow(window);
					appendCaption(info);
					lastSeqNum = seqNum;

					//	Clear state info...
					seqNum = 0;
					startTS = endTS = 0;
					durationMS = 0;
					captionText.clear();
					window.clear();
					anchorPos = 0;
					state  = kStateSeqNum;
					break;
				}
				if (!captionText.empty())
					captionText += " ";
				captionText += line;
				break;

			default:
				NTV2_ASSERT(false);
				break;
		}	//	switch
		if (!mErrors.empty())
			break;
	}	//	for each line
	if (state == kStateCaption)
	{
		SRTCaptionInfo info (seqNum,  startTS,  durationMS,  captionText, startSrcLine, mLinesParsed);
		if (isEmpty())
			info.setFirstCaption();
		if (anchorPos)
			NTV2_ASSERT(anchorPos  && !window.isValid());
		if (window.isValid())
			NTV2_ASSERT(!anchorPos  && window.isValid());
		if (anchorPos)
			info.setAnchorPos(anchorPos);
		if (window.isValid())
			info.setWindow(window);
		appendCaption(info);
	}
	return mErrors.empty();
}	//	reloadFrom

bool SRTCaptions::reloadFrom (const string & inSRTData)
{
	return reloadFrom(aja::split(inSRTData, "\n"));
}

void SRTCaptions::clear (void)
{
	mTimestamps.clear();
	mSeqNums.clear();
	mCaptions.clear();
	mDurations.clear();
	mWindows.clear();
	mAnchors.clear();
	mStartLines.clear();
	mEndLines.clear();
	mErrors.clear();
	mWarnings.clear();
	mLinesParsed = 0;
	mLines.clear();
}

ostream & SRTCaptions::printErrors (ostream & oss, const bool inIncludeWarnings) const
{
	if (inIncludeWarnings  &&  hasWarnings())
	{
		oss << DEC(warningCount()) << " line(s) have warning(s):" << endl;
		string warnMsg;
		for (size_t warnNdx(0), lineNum0(0);  getWarning (warnNdx, lineNum0, warnMsg);  warnNdx++)
			oss << "L" << DEC0N(lineNum0+1,3) << ": " << lineAtOffset(lineNum0) << endl
				<< "WARNING:  " << warnMsg << endl;
	}
	if (!hasErrors())
		return oss;
	size_t lineNum(0); string errMsg;
	if (getError(lineNum, errMsg))
		oss << "Parse failed in 'reloadFrom' in SRT source:" << endl
			<< "L" << DEC0N(lineNum+1,3) << ": " << lineAtOffset(lineNum) << endl
			<< "ERROR:  " << errMsg << endl;
	return oss;
}

ostream & SRTCaptions::printSRT (ostream & oss) const
{
	SRTCaptionInfo info;
	for (SRTTimestamp ts(0);  nextCaption(ts, info);  )
		info.printSRT(oss);
	return oss;
}

ostream & SRTCaptions::printInfo (ostream & inOutStrm) const
{
	AJALabelValuePairs info;
	{ostringstream oss;  oss << DEC(linesParsed()) << " of " << DEC(totalLines()); AJASystemInfo::append(info, "Lines Parsed", oss.str());}
	{ostringstream oss;  oss << DEC(warningCount());	AJASystemInfo::append(info, "Warning Count", oss.str());}
	{ostringstream oss;  oss << DEC(errorCount());	AJASystemInfo::append(info, "Error Count", oss.str());}
	{ostringstream oss;  oss << DEC(captionCount());	AJASystemInfo::append(info, "Caption Count", oss.str());}
	{ostringstream oss;  oss << DEC(minSeqNum()) << " thru " << DEC(maxSeqNum()); AJASystemInfo::append(info, "Sequence Range", oss.str());}
	{ostringstream oss;  oss << timestampStr (minStartTime(),true);	AJASystemInfo::append(info, "First Timestamp", oss.str());}
	{ostringstream oss;  oss << timestampStr (maxStartTime(),true);	AJASystemInfo::append(info, "Last Timestamp", oss.str());}
	{ostringstream oss;  oss << durationStr (totalDuration(),true);	AJASystemInfo::append(info, "Total Duration", oss.str());}
	inOutStrm << AJASystemInfo::ToString(info);
	return inOutStrm;
}

uint32_t SRTCaptions::captionCount (void) const
{
	return mCaptions.size();
}

SRTTimestamp SRTCaptions::minStartTime (void) const
{
	CaptionMapConstIter it(mCaptions.begin());
	return it != mCaptions.end() ? it->first : 0;
}

SRTTimestamp SRTCaptions::maxStartTime (void) const
{
	CaptionMap::const_reverse_iterator rit(mCaptions.rbegin());
	return rit != mCaptions.rend() ? rit->first : 0;
}

SRTSeqNum SRTCaptions::minSeqNum (void) const
{
	TimestampMapConstIter it(mTimestamps.begin());
	return it != mTimestamps.end() ? it->first : 0;
}

SRTSeqNum SRTCaptions::maxSeqNum (void) const
{
	TimestampMap::const_reverse_iterator rit(mTimestamps.rbegin());
	return rit != mTimestamps.rend() ? rit->first : 0;
}

bool SRTCaptions::appendCaption (const SRTCaptionInfo & inCaptionInfo)
{
	const SRTTimestamp startTime(inCaptionInfo.startTime());
	if (hasCaptionAtStartTime(startTime))
		return false;	//	Already has caption at that start time
	if (!inCaptionInfo.sequenceNum())
		return false;	//	Bad (zero) sequence number
	mTimestamps[inCaptionInfo.sequenceNum()] = startTime;
	mSeqNums[startTime] = inCaptionInfo.sequenceNum();
	mCaptions[startTime] = inCaptionInfo.rawCaption();
	mDurations[startTime] = inCaptionInfo.duration();
	if (inCaptionInfo.window().isValid())
		mWindows[startTime] = inCaptionInfo.window();
	if (inCaptionInfo.hasAnchor())
		mAnchors[startTime] = inCaptionInfo.anchor();
	if (inCaptionInfo.hasLineNumbers())
	{
		mStartLines[startTime] = inCaptionInfo.startLineNumber();
		mEndLines[startTime] = inCaptionInfo.endLineNumber();
	}
	return true;
}

bool SRTCaptions::hasCaptionAtSeqNum (const SRTSeqNum inSeqNum) const
{
	return mTimestamps.find(inSeqNum) != mTimestamps.end();
}

bool SRTCaptions::hasCaptionAtStartTime (const SRTTimestamp inStartTime) const
{
	return mCaptions.find(inStartTime) != mCaptions.end();
}

SRTTimestamp SRTCaptions::startTimeAtSeqNum (const SRTSeqNum inSeqNum) const
{
	TimestampMapConstIter it(mTimestamps.find(inSeqNum));
	if (it == mTimestamps.end())
		return 0;
	return it->second;
}

bool SRTCaptions::captionAtStartTime (const SRTTimestamp inStartTime, SRTCaptionInfo & outInfo) const
{
	outInfo.reset();
	CaptionMapConstIter itCaption(mCaptions.find(inStartTime));
	SeqNumMapConstIter itSeqNum(mSeqNums.find(inStartTime));
	DurationMapConstIter itDuration(mDurations.find(inStartTime));
	WindowMapConstIter itWndo(mWindows.find(inStartTime));
	AnchorMapConstIter itAnchor(mAnchors.find(inStartTime));
	SourceMapConstIter itStartLine(mStartLines.find(inStartTime));
	SourceMapConstIter itEndLine(mEndLines.find(inStartTime));
	if (itCaption == mCaptions.end())
		return false;
	if (itSeqNum == mSeqNums.end())
		return false;
	if (itDuration == mDurations.end())
		return false;
	outInfo.set(itSeqNum->second, inStartTime, itDuration->second, itCaption->second);
	if (itCaption == mCaptions.begin())
		outInfo.setFirstCaption();
	if (itWndo != mWindows.end())
		outInfo.setWindow(itWndo->second);
	if (itAnchor != mAnchors.end())
		outInfo.setAnchorPos(itAnchor->second);
	if (itStartLine != mStartLines.end()  &&  itEndLine != mEndLines.end())
		outInfo.setLineNumbers(itStartLine->second, itEndLine->second);
	return true;
}

string SRTCaptions::captionAtStartTime (const SRTTimestamp inStartTime) const
{
	CaptionMapConstIter itCaption(mCaptions.find(inStartTime));
	if (itCaption == mCaptions.end())
		return "";
	return itCaption->second;
}

bool SRTCaptions::durationAtStartTime (const SRTTimestamp inStartTime, SRTDuration & outDuration) const
{
	outDuration = 0;
	DurationMapConstIter itDuration(mDurations.find(inStartTime));
	if (itDuration == mDurations.end())
		return false;
	outDuration = itDuration->second;
	return true;
}

//	Upon entry, startTime is current position in caption list
//	Upon exit, startTime is current position in caption list after advancing to next caption (or zero if past end)
bool SRTCaptions::nextCaption (SRTTimestamp & inOutStartTime, SRTCaptionInfo & outInfo) const
{
	outInfo.set(/*seqNum*/0, /*startTime*/0, /*duration*/0);
	while (true)
	{
		if (inOutStartTime == maxStartTime())	//	At end?
			{inOutStartTime = 0;  break;}		//	Done!
		CaptionMapConstIter it;
		if (!inOutStartTime)	//	Start at beginning?
		{
			inOutStartTime = minStartTime();	//	First
			captionAtStartTime(inOutStartTime, outInfo);
			break;
		}
		it = mCaptions.find(inOutStartTime);	//	Current position
		if (it == mCaptions.end())
			{inOutStartTime = 0;  break;}	//	Not found

		//	Advance to next caption...
		++it;
		if (it == mCaptions.end())
			{inOutStartTime = 0;  break;}	//	Past end
		inOutStartTime = it->first;
		captionAtStartTime(inOutStartTime, outInfo);
		break;
	}
	return outInfo.isValid();
}

bool SRTCaptions::getWarning (const size_t inIndex0, size_t & outLineOffset, std::string & outMsg) const
{
	outMsg.clear();
	outLineOffset = 0;
	size_t ndx(0);
	for (ErrorMapConstIter it(mWarnings.begin());  it != mWarnings.end();  ++it)
		if (ndx++ == inIndex0)
		{
			outLineOffset = it->first;
			outMsg = it->second;
			return true;
		}
	return false;
}

bool SRTCaptions::getError (size_t & outLineOffset, std::string & outMsg) const
{
	outMsg.clear();
	outLineOffset = 0;
	if (!hasErrors())
		return false;
	outLineOffset = mErrors.begin()->first;
	outMsg = mErrors.begin()->second;
	return !outMsg.empty();
}
