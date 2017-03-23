/**
	@file		ancillarydata.h
	@brief		Declares the AJAAncillaryData class.
	@copyright	(C) 2010-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef AJA_ANCILLARYDATA_H
#define AJA_ANCILLARYDATA_H

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ajaexport.h"
#include "ajabase/common/common.h"
#include <sstream>


/**
	@page	ajaanc		The AJA Ancillary Data Library

	The AJA Ancillary Data Library (AJAAncLib) is a suite of classes and data types which allow end-users to easily encode or decode
	ancillary data using nearly any NTV2-compatible AJA device using the C++ programming language.
	The code operates on Windows/VisualStudio, MacOS/Xcode and Linux/gcc.

	The purpose of the library is to enable third-parties to easily access and/or control the ancillary data entering or leaving
	an AJA NTV2 device. The library currently is very focused on CEA-608.

	For examples on how to use this library, please see the \ref ntv2ccgrabber and \ref ntv2ccplayer demonstration applications.

	<b>Principal Classes</b>
	- AJAAncillaryData:  Container that holds a single Anc packet (or scan line of "analog" anc data).
		- AJAAncillaryData_Cea608:  A CEA-608 data packet.
			- AJAAncillaryData_Cea608_Line21:  An "analog" CEA-608 data packet (decoded from or encoded to line 21).
			- AJAAncillaryData_Cea608_Vanc:  A CEA-608 data packet sourced from or destined to a frame buffer using a tall (or taller) frame geometry.
		- AJAAncillaryData_Cea708:  Container that holds a single CEA-708 SMPTE 334 packet.
		- AJAAncillaryData_Timecode:  Container that holds a single timecode packet.
			- AJAAncillaryData_Timecode_ATC:  An "analog" (ATC) timecode packet.
			- AJAAncillaryData_Timecode_VITC:  A VITC timecode packet.
		- AJAAncillaryData_FrameStatusInfo524D:  A "524D" frame status info packet.
		- AJAAncillaryData_FrameStatusInfo5251:  A "5251" frame status info packet.
	- AJAAncillaryDataFactory:  Provides AJAAncillaryDataFactory::Create and AJAAncillaryDataFactory::GuessAncillaryDataType class methods.
	- AJAAncillaryList:  An ordered collection of AJAAncillaryData packets.
**/


// Default Packet IDs used when building "analog" packets
// NOTE: there is NO guarantee that the Anc Extractor hardware will use these codes - nor does the
//       Anc Inserter hardware care. If you want to know whether a given packet is "analog" or
//		 "digital", you should look at the appropriate flag in the packet header. I just wanted to
//		 have all the locally (software) built "analog" IDs come from a single location.
const uint8_t AJAAncillaryData_AnalogDID = 0x00;
const uint8_t AJAAncillaryData_AnalogSID = 0x00;


typedef std::pair <uint8_t, uint8_t>			AJAAncillaryDIDSIDPair;		///< @brief	A DID/SID pair, typically used as an indexing key.

/**
	@brief	Writes a human-readable rendition of the given AJAAncillaryDIDSIDPair into the given output stream.
	@param		inOutStream		Specifies the output stream to be written.
	@param[in]	inData			Specifies the AJAAncillaryDIDSIDPair to be rendered into the output stream.
	@return		A non-constant reference to the specified output stream.
**/
AJAExport std::ostream & operator << (std::ostream & inOutStream, const AJAAncillaryDIDSIDPair & inData);


/**
	@brief	Identifies the ancillary data types that are known to this module.
**/
enum AJAAncillaryDataType
{
	AJAAncillaryDataType_Unknown,				///< @brief	Includes data that is valid, but we don't recognize

	AJAAncillaryDataType_Smpte2016_3,			///< @brief	SMPTE 2016-3 VANC Aspect Format Description (AFD) metadata
	AJAAncillaryDataType_Timecode_ATC,			///< @brief	SMPTE 12-M Ancillary Timecode (formerly known as "RP-188")
	AJAAncillaryDataType_Timecode_VITC,			///< @brief	SMPTE 12-M Vertical Interval Timecode (aka "VITC")
	AJAAncillaryDataType_Cea708,				///< @brief	CEA708 (SMPTE 334) HD Closed Captioning
	AJAAncillaryDataType_Cea608_Vanc,			///< @brief	CEA608 SD Closed Captioning (SMPTE 334 VANC packet)
	AJAAncillaryDataType_Cea608_Line21,			///< @brief	CEA608 SD Closed Captioning ("Line 21" waveform)
	AJAAncillaryDataType_Smpte352,				///< @brief	SMPTE 352 "Payload ID"
	AJAAncillaryDataType_Smpte2051,				///< @brief	SMPTE 2051 "Two Frame Marker"
	AJAAncillaryDataType_FrameStatusInfo524D,	///< @brief	Frame Status Information, such as Active Frame flag
	AJAAncillaryDataType_FrameStatusInfo5251,	///< @brief	Frame Status Information, such as Active Frame flag
	AJAAncillaryDataType_Size
};

#define	IS_VALID_AJAAncillaryDataType(_x_)		((_x_) >= AJAAncillaryDataType_Unknown  &&  (_x_) < AJAAncillaryDataType_Size)

/**
	@return		A string containing a human-readable representation of the given AJAAncillaryDataType value (or empty if not possible).
	@param[in]	inValue		Specifies the AJAAncillaryDataType value to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise use the longer symbolic format.
**/
AJAExport const std::string & AJAAncillaryDataTypeToString (const AJAAncillaryDataType inValue, const bool inCompact = true);



/**
	@brief	Identifies which link of a video stream the ancillary data is associated with.
**/
enum AJAAncillaryDataLink
{
	AJAAncillaryDataLink_A,			///< @brief	The ancillary data is associated with Link A of the video stream.
	AJAAncillaryDataLink_B,			///< @brief	The ancillary data is associated with Link B of the video stream.

	AJAAncillaryDataLink_Unknown,	///< @brief	It is not known which link of the video stream the ancillary data is associated with.
	AJAAncillaryDataLink_Size
};

#define	IS_VALID_AJAAncillaryDataLink(_x_)		((_x_) >= AJAAncillaryDataLink_A  &&  (_x_) < AJAAncillaryDataLink_Unknown)

/**
	@return		A string containing a human-readable representation of the given AJAAncillaryDataLink value (or empty if not possible).
	@param[in]	inValue		Specifies the AJAAncillaryDataLink value to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise use the longer symbolic format.
**/
AJAExport const std::string &	AJAAncillaryDataLinkToString (const AJAAncillaryDataLink inValue, const bool inCompact = true);


/**
	@brief	Identifies which component of a video stream in which the ancillary data is placed or found.
**/
enum AJAAncillaryDataVideoStream
{
	AJAAncillaryDataVideoStream_C,		///< @brief	The ancillary data is associated with the chrominance (C) component of the video stream.
	AJAAncillaryDataVideoStream_Y,		///< @brief	The ancillary data is associated with the luminance (Y) component of the video stream.

	AJAAncillaryDataVideoStream_Unknown,	///< @brief	It is not known which component of the video stream the ancillary data is associated with.
	AJAAncillaryDataVideoStream_Size
};

#define	IS_VALID_AJAAncillaryDataVideoStream(_x_)		((_x_) >= AJAAncillaryDataVideoStream_C  &&  (_x_) < AJAAncillaryDataVideoStream_Unknown)

/**
	@return		A string containing a human-readable representation of the given AJAAncillaryDataVideoStream value (or empty if not possible).
	@param[in]	inValue		Specifies the AJAAncillaryDataVideoStream value to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise use the longer symbolic format.
**/
AJAExport const std::string &	AJAAncillaryDataVideoStreamToString (const AJAAncillaryDataVideoStream inValue, const bool inCompact = true);


/**
	@brief	Identifies the raster section of a video stream that contains the ancillary data.
**/
enum AJAAncillaryDataSpace
{
	AJAAncillaryDataSpace_VANC,		///< @brief	The ancillary data is found between SAV and EAV (in the vertical blanking area).
	AJAAncillaryDataSpace_HANC,		///< @brief	The ancillary data is found between EAV and SAV (in the horizontal blanking area).

	AJAAncillaryDataSpace_Unknown,	///< @brief	It is not known which raster section of a video stream the ancillary data is contain within.
	AJAAncillaryDataSpace_Size
};

#define	IS_VALID_AJAAncillaryDataSpace(_x_)		((_x_) >= AJAAncillaryDataSpace_VANC  &&  (_x_) < AJAAncillaryDataSpace_Unknown)

/**
	@return		A string containing a human-readable representation of the given AJAAncillaryDataSpace value (or empty if not possible).
	@param[in]	inValue		Specifies the AJAAncillaryDataSpace value to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise use the longer symbolic format.
**/
AJAExport const std::string &	AJAAncillaryDataSpaceToString (const AJAAncillaryDataSpace inValue, const bool inCompact = true);


/**
	@brief	Defines where the ancillary data can be found within a video stream.
**/
typedef struct AJAAncillaryDataLocation
{
	public:
		inline	AJAAncillaryDataLocation (	const AJAAncillaryDataLink			inLink		= AJAAncillaryDataLink_Unknown,
											const AJAAncillaryDataVideoStream	inStream	= AJAAncillaryDataVideoStream_Unknown,
											const AJAAncillaryDataSpace			inAncSpace	= AJAAncillaryDataSpace_Unknown,
											const uint16_t						inLineNum	= 0)
			:	link		(inLink),
				stream		(inStream),
				ancSpace	(inAncSpace),
				lineNum		(inLineNum)
				{}

		inline bool		operator == (const AJAAncillaryDataLocation & inRHS) const
		{
			return link == inRHS.link && stream == inRHS.stream && ancSpace == inRHS.ancSpace && lineNum == inRHS.lineNum;
		}

		inline bool		operator < (const AJAAncillaryDataLocation & inRHS) const
		{
			const uint32_t	lhs	((link << 12) | (stream << 10) | (ancSpace << 8) | lineNum);
			const uint32_t	rhs	((inRHS.link << 12) | (inRHS.stream << 10) | (inRHS.ancSpace << 8) | inRHS.lineNum);
			return lhs < rhs;
		}

		inline bool		IsValid (void) const
		{
			return IS_VALID_AJAAncillaryDataLink (link) && IS_VALID_AJAAncillaryDataVideoStream (stream) && IS_VALID_AJAAncillaryDataSpace (ancSpace);
		}

		inline void		Set (	const AJAAncillaryDataLink			inLink,
								const AJAAncillaryDataVideoStream	inStream,
								const AJAAncillaryDataSpace			inAncSpace,
								const uint16_t						inLineNum)
		{
			link		= inLink;
			stream		= inStream;
			ancSpace	= inAncSpace;
			lineNum		= inLineNum;
		}

		inline void		Reset (void)
		{
			link		= AJAAncillaryDataLink_Unknown;
			stream		= AJAAncillaryDataVideoStream_Unknown;
			ancSpace	= AJAAncillaryDataSpace_Unknown;
			lineNum		= 0;
		}

		inline AJAAncillaryDataLink			GetDataLink (void) const				{return link;}
		inline bool							IsDataLinkA (void) const				{return link == AJAAncillaryDataLink_A;}
		inline bool							IsDataLinkB (void) const				{return link == AJAAncillaryDataLink_B;}

		inline AJAAncillaryDataVideoStream	GetDataStream (void) const				{return stream;}
		inline bool							IsLumaChannel (void) const				{return stream == AJAAncillaryDataVideoStream_Y;}
		inline bool							IsChromaChannel (void) const			{return stream == AJAAncillaryDataVideoStream_C;}

		inline AJAAncillaryDataSpace		GetDataSpace (void) const				{return ancSpace;}
		inline bool							IsVanc (void) const						{return ancSpace == AJAAncillaryDataSpace_VANC;}
		inline bool							IsHanc (void) const						{return ancSpace == AJAAncillaryDataSpace_HANC;}

		inline uint16_t						GetLineNumber (void) const				{return lineNum;}

		/**
			@brief	Sets my data link value to the given value (if valid).
			@param[in]	inLink		Specifies the new data link value to use. Must be valid.
			@return	A non-const reference to myself.
		**/
		inline AJAAncillaryDataLocation &	SetDataLink (const AJAAncillaryDataLink inLink)						{if (IS_VALID_AJAAncillaryDataLink (inLink)) link = inLink;	return *this;}

		/**
			@brief	Sets my data video stream value to the given value (if valid).
			@param[in]	inStream	Specifies the new data stream value to use. Must be valid.
			@return	A non-const reference to myself.
		**/
		inline AJAAncillaryDataLocation &	SetDataVideoStream (const AJAAncillaryDataVideoStream inStream)		{if (IS_VALID_AJAAncillaryDataVideoStream (inStream)) stream = inStream; return *this;}

		/**
			@brief	Sets my data space value to the given value (if valid).
			@param[in]	inSpace		Specifies the new data space value to use. Must be valid.
			@return	A non-const reference to myself.
		**/
		inline AJAAncillaryDataLocation &	SetDataSpace (const AJAAncillaryDataSpace inSpace)					{if (IS_VALID_AJAAncillaryDataSpace (inSpace)) ancSpace = inSpace; return *this;}

		/**
			@brief	Sets my anc data line number value.
			@param[in]	inLineNum		Specifies the new line number value to use.
			@return	A non-const reference to myself.
		**/
		inline AJAAncillaryDataLocation &	SetLineNumber (const uint16_t inLineNum)							{lineNum = inLineNum; return *this;}

	AJAAncillaryDataLink			link;		///< @brief	Which data link (A or B)?
	AJAAncillaryDataVideoStream		stream;		///< @brief	Which component stream (Y or C)?
	AJAAncillaryDataSpace			ancSpace;	///< @brief	Which raster section (HANC or VANC)?
	uint16_t						lineNum;	///< @brief	Which SMPTE line number?

} AJAAncillaryDataLocation, *AJAAncillaryDataLocationPtr;


/**
	@return		A string containing a human-readable representation of the given AJAAncillaryDataLocation value (or empty if invalid).
	@param[in]	inValue		Specifies the AJAAncillaryDataLocation to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise use the longer symbolic format.
**/
AJAExport std::string	AJAAncillaryDataLocationToString (const AJAAncillaryDataLocation & inValue, const bool inCompact = true);


/**
	@brief	Writes a human-readable rendition of the given AJAAncillaryDataLocation into the given output stream.
	@param		inOutStream		Specifies the output stream to be written.
	@param[in]	inData			Specifies the AJAAncillaryDataLocation to be rendered into the output stream.
	@return		A non-constant reference to the specified output stream.
**/
AJAExport std::ostream & operator << (std::ostream & inOutStream, const AJAAncillaryDataLocation & inData);


/**
	@brief	Identifies the ancillary data coding type:  digital or non-digital (analog/raw).
**/
enum AJAAncillaryDataCoding
{
	AJAAncillaryDataCoding_Digital,		///< @brief	The ancillary data is in the form of a SMPTE-291 Ancillary Packet.
	AJAAncillaryDataCoding_Raw,			///< @brief	The ancillary data is in the form of a digitized waveform (e.g. CEA-608 captions, VITC, etc.).
	AJAAncillaryDataCoding_Analog	= AJAAncillaryDataCoding_Raw,	///< @deprecated	Use AJAAncillaryDataCoding_Raw instead.
	AJAAncillaryDataCoding_Unknown,		///< @brief	It is not known which coding type the ancillary data is using.
	AJAAncillaryDataCoding_Size
};

#define	IS_VALID_AJAAncillaryDataCoding(_x_)		((_x_) >= AJAAncillaryDataCoding_Digital  &&  (_x_) < AJAAncillaryDataCoding_Size)

/**
	@return		A string containing a human-readable representation of the given AJAAncillaryDataLocation value (or empty if invalid).
	@param[in]	inValue		Specifies the AJAAncillaryDataLocation to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise use the longer symbolic format.
**/
AJAExport const std::string &	AJAAncillaryDataCodingToString (const AJAAncillaryDataCoding inValue, const bool inCompact = true);


/**
	@brief	An AJAAncillaryData object typically holds the contents of one (1) SMPTE-291 SDI
			ancillary packet OR the digitized contents of one (1) "analog" scan line (e.g. line 21
			captions or VITC). While AJAAncillaryData is payload-agnostic, it can serve as the
			generic base class for more specific objects that know how to decode/parse given types
			of ancillary data.
**/
class AJAExport AJAAncillaryData
{
public:
	AJAAncillaryData ();	///< @brief	My default constructor.

	/**
		@brief	My copy constructor.
		@param[in]	inClone	The AJAAncillaryData object to be cloned.
	**/
	AJAAncillaryData (const AJAAncillaryData & inClone);

	/**
		@brief	My copy constructor.
		@param[in]	pInClone	A valid pointer to the AJAAncillaryData object to be cloned.
	**/
	AJAAncillaryData (const AJAAncillaryData * pInClone);

	virtual									~AJAAncillaryData ();	///< @brief		My destructor.

	virtual void							Clear (void);			///< @brief	Frees my allocated memory, if any, and resets my members to their default values.


	/**
		@brief	Assignment operator -- replaces my contents with the right-hand-side value.
		@param[in]	inRHS	The value to be assigned to me.
		@return		A reference to myself.
	**/
	virtual AJAAncillaryData &				operator = (const AJAAncillaryData & inRHS);

	virtual AJAAncillaryData *				Clone (void) const;	//	@return	A clone of myself.


	/**
		@brief		Sets my Data ID (DID).
		@param[in]	inDataID	Specifies my new Data ID (for digital ancillary data, usually the "official" SMPTE packet ID).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetDID (const uint8_t inDataID);

	virtual inline uint8_t					GetDID (void) const					{return m_DID;}		///< @return	My Data ID (DID).


	/**
		@brief		Sets/Gets the Secondary Data ID (SID) - (aka the Data Block Number (DBN) for "Type 1" SMPTE-291 packets).
		@param[in]	inSID		Secondary Data ID (for digital ancillary data, usually the "official" SMPTE packet ID)
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetSID (const uint8_t inSID);

	virtual inline uint8_t					GetSID (void) const					{return m_SID;}		///< @return	My secondary data ID (SID).

	virtual inline uint32_t					GetDC (void) const					{return m_DC;}		///< @return	My payload data count, in bytes.


	/**
		@brief		Sets my 8-bit checksum. Note that it is not usually necessary to generate an 8-bit checksum, since the ANC Insertion
					hardware ignores this field and (for SMPTE-291 Anc packets) generates and inserts its own "proper" 9-bit SMPTE-291 checksum.
		@param[in]	checksum8	8-bit checksum
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetChecksum (const uint8_t checksum8)	{m_checksum = checksum8;  return AJA_STATUS_SUCCESS;}

	virtual uint8_t							GetChecksum (void) const				{return m_checksum;}	///< @return	My 8-bit checksum.

	/**
		@brief	Generates an 8-bit checksum from the DID + SID + DC + payload data.
		@note	This is NOT the same as the official SMPTE-291 checksum, which is 9 bits wide and should be calculated by the hardware.
		@return		The calculated 8-bit checksum.
	**/
	virtual uint8_t							Calculate8BitChecksum (void) const;


	/**
		@brief	Compares the received 8-bit checksum with a newly calculated 8-bit checksum. Returns 'true' if they match.
		@note	This is NOT the same as the official SMPTE-291 checksum, which is 9 bits wide and should be calculated by the hardware.
		@return	True if the calculated checksum matches received checksum; otherwise false.
	**/
	virtual bool							ChecksumOK (void) const				{return m_checksum == Calculate8BitChecksum ();}



	/**
		@brief		Sets my ancillary data "location" within the video stream.
		@param[in]	inLoc		Specifies the new AJAAncillaryDataLocation value.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetDataLocation (const AJAAncillaryDataLocation & inLoc);

	virtual inline AJAStatus				GetDataLocation (AJAAncillaryDataLocation & outLocInfo) const		///< @deprecated	Use the inline version instead.
																						{outLocInfo = GetDataLocation ();	return AJA_STATUS_SUCCESS;}
	virtual AJAStatus						GetDataLocation (AJAAncillaryDataLink & link,
															AJAAncillaryDataVideoStream & stream,
															AJAAncillaryDataSpace & ancSpace,
															uint16_t & lineNum);			///< @deprecated	Use the inline GetDataLocation() instead.
	virtual inline const AJAAncillaryDataLocation &		GetDataLocation (void) const		{return m_location;}	///< @brief	My ancillary data "location" within the video stream.

	/**
		@brief		Sets my ancillary data "location" within the video stream.
		@param[in]	inLink		Specifies the video link (A or B).
		@param[in]	inStream	Specifies the video stream (Y or C).
		@param[in]	inAncSpace	Specifies the ancillary data space (HANC or VANC).
		@param[in]	inLineNum	Specifies the frame line number (SMPTE line numbering).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetDataLocation (const AJAAncillaryDataLink inLink, const AJAAncillaryDataVideoStream inStream, const AJAAncillaryDataSpace inAncSpace, const uint16_t inLineNum);


	/**
		@brief		Sets my ancillary data "location" within the video stream.
		@param[in]	inLink	Specifies the new video link value (A or B).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetLocationVideoLink (const AJAAncillaryDataLink inLink);

	virtual inline AJAAncillaryDataLink		GetLocationVideoLink (void) const			{return m_location.link;}	///< @return	My current anc data video link value (A or B).

	/**
		@brief	Sets my ancillary data "location" video stream value (Y or C).
		@param[in]	inStream	Specifies my new video stream (Y or C) value.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus SetLocationVideoStream (const AJAAncillaryDataVideoStream inStream);

	virtual inline AJAAncillaryDataVideoStream	GetLocationVideoStream (void) const		{return m_location.stream;}	///< @return	My current anc data location's video stream (Y or C).

	/**
		@brief		Sets my ancillary data "location" data space value.
		@param[in]	inAncSpace	Specifies the new ancillary data space value (HANC or VANC).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetLocationVideoSpace (const AJAAncillaryDataSpace inAncSpace);

	virtual inline AJAAncillaryDataSpace	GetLocationVideoSpace (void) const			{return m_location.ancSpace;}				///< @return	My current ancillary data space (HANC or VANC).

	virtual inline bool						IsLumaChannel (void) const					{return m_location.IsLumaChannel ();}		///< @return	True if my location component stream is Y (luma).
	virtual inline bool						IsChromaChannel (void) const				{return m_location.IsChromaChannel ();}		///< @return	True if my location component stream is C (chroma).
	virtual inline bool						IsVanc (void) const							{return m_location.IsVanc ();}				///< @return	True if my location data space is VANC.
	virtual inline bool						IsHanc (void) const							{return m_location.IsHanc ();}				///< @return	True if my location data space is HANC.
	virtual inline bool						IsDigital (void) const						{return m_coding == AJAAncillaryDataCoding_Digital;}	///< @return	True if my coding type is digital.
	virtual inline bool						IsRaw (void) const							{return m_coding == AJAAncillaryDataCoding_Raw;}		///< @return	True if my coding type is "raw" (i.e., from an digitized waveform).
	virtual inline bool						IsAnalog (void) const						{return m_coding == AJAAncillaryDataCoding_Raw;}		///< @deprecated	Use IsRaw instead.

	/**
		@brief	Sets my ancillary data "location" frame line number.
		@param[in]	inLineNum	Specifies the new frame line number value (SMPTE line numbering).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetLocationLineNumber (const uint16_t inLineNum);

	virtual inline uint16_t					GetLocationLineNumber (void) const			{return m_location.lineNum;}	///< @return	My current frame line number value (SMPTE line numbering).

	/**
		@brief		Sets my ancillary data coding type (e.g. digital or analog/raw waveform).
		@param[in]	inCodingType	AJAAncillaryDataCoding
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetDataCoding (const AJAAncillaryDataCoding inCodingType);

	virtual inline AJAAncillaryDataCoding	GetDataCoding (void) const					{return m_coding;}	///< @return	The ancillary data coding type (e.g., digital or analog/raw waveform).


	/**
		@brief		Copy data from external memory into my local payload memory.
		@param[in]	pInData		Specifies the address of the first byte of the external payload data to be copied (source).
		@param[in]	inByteCount	Specifies the number of bytes of payload data to be copied.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetPayloadData (const uint8_t * pInData, const uint32_t inByteCount);

	/**
		@brief		Appends data from an external buffer onto the end of my existing payload.
		@param[in]	pInBuffer		Specifies a valid, non-NULL starting address of the external buffer from which the payload data will be copied.
		@param[in]	inByteCount		Specifies the number of bytes to append.
		@return		AJA_STATUS_SUCCESS
	**/
	virtual AJAStatus						AppendPayloadData (const uint8_t * pInBuffer, const uint32_t inByteCount);

	/**
		@brief		Appends payload data from another AJAAncillaryData object to my existing payload.
		@param[in]	inAncData	The AJAAncillaryData object whose payload data is to be appended to my own.
		@return		AJA_STATUS_SUCCESS
	**/
	virtual AJAStatus						AppendPayload (const AJAAncillaryData & inAncData);

	/**
		@deprecated	Use AppendPayload(const AJAAncillaryData &) instead.
	**/
	virtual inline AJAStatus				AppendPayload (const AJAAncillaryData * pInAncData)		{return pInAncData ? AppendPayload (*pInAncData) : AJA_STATUS_NULL;}


	/**
	 	@brief	Copy data from external memory to local payload memory.
		@param[in]	pInData		A valid, non-NULL pointer to the external payload data to be copied (source).
								The upper 8 bits of each 16-bit word will be skipped and ignored.
		@param[in]	inNumWords	Specifies the number of 16-bit words of payload data to copy.
		@param[in]	inLocInfo	Specifies the anc data location information.
		@return					AJA_STATUS_SUCCESS
	**/
	virtual AJAStatus						SetFromSMPTE334 (const uint16_t * pInData, const uint32_t inNumWords, const AJAAncillaryDataLocation & inLocInfo);

	/**
		@brief	Copies my payload data into an external buffer.
		@param[in]	pBuffer			Specifies a valid, non-null starting address to where the payload data is to be copied.
		@param[in]	inByteCapacity	Specifies the maximum number of bytes that can be safely copied into the external buffer.
		@return		AJA_STATUS_SUCCESS
	**/
	virtual AJAStatus						GetPayloadData (uint8_t * pBuffer, const uint32_t inByteCapacity) const;

	/**
		@param[in]	inIndex0	Specifies the zero-based index value. This should be less than GetPayloadByteCount's result.
		@return		The payload data byte at the given zero-based index (or zero if the index value is invalid).
	**/
	virtual uint8_t							GetPayloadByteAtIndex (const uint32_t inIndex0) const;

	virtual AJAAncillaryDataType			GetAncillaryDataType (void) const		{return m_ancType;}	///< @return	My anc data type (if known).


	/**
		@brief		Parses out (interprets) the "local" ancillary data from my payload data.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						ParsePayloadData (void);


	/**
		@brief		Used following ParsePayloadData() to determine whether or not the object thinks it contains
					valid ancillary data.
		@note		The "depth" of the ParsePayloadData method depends on the packet type. In some cases, 'true'
					just means "I see some data there", in others there is more detailed data checking.
		@return		True if I think I have valid ancillary data;  otherwise false.
	**/
	virtual inline bool						GotValidReceiveData (void) const			{return m_rcvDataValid;}


	/**
		@brief		Generate the payload data from the "local" ancillary data.
		@note		This method is overridden for the specific Anc data type.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual inline AJAStatus				GeneratePayloadData (void)					{return AJA_STATUS_SUCCESS;}


	/**
		@brief		Initializes me from "raw" ancillary data received from hardware (ingest).
		@param[in]	pInData				Specifies the starting address of the "raw" packet data that was received from the AJA device.
		@param[in]	inMaxBytes			Specifies the maximum number of bytes left in the source buffer.
		@param[in]	inLocationInfo		Specifies the default location info.
		@param[out]	outPacketByteCount	Receives the size (in bytes) of the parsed packet.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						InitWithReceivedData (const uint8_t * pInData, const uint32_t inMaxBytes, const AJAAncillaryDataLocation & inLocationInfo, uint32_t & outPacketByteCount);

	/**
		@brief		Initializes me from "raw" ancillary data received from hardware (ingest).
		@param[in]	inData				Specifies the "raw" packet data.
		@param[in]	inLocationInfo		Specifies the default location info.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						InitWithReceivedData (const std::vector<uint8_t> & inData, const AJAAncillaryDataLocation & inLocationInfo);

	/**
		@deprecated	Use InitWithReceivedData(const uint8_t *, const uint32_t, const AJAAncillaryDataLocation &, uint32_t &) instead.
	**/
	virtual inline AJAStatus				InitWithReceivedData (const uint8_t * pInData, const uint32_t inMaxBytes, const AJAAncillaryDataLocation * pInLoc, uint32_t & outPacketByteCount)
	{
		return pInLoc ? InitWithReceivedData (pInData, inMaxBytes, *pInLoc, outPacketByteCount) : AJA_STATUS_NULL;
	}


	/**
		@brief		Returns the number of "raw" ancillary data bytes that will be generated by GenerateTransmitData() (playback).
		@param[out]	outPacketSize	Receives the size (in bytes) of the packet I will generate.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetRawPacketSize (uint32_t & outPacketSize);

	/**
		@brief		Generates "raw" ancillary data from my internal ancillary data (playback).
		@param[in]	pData			Pointer to "raw" packet data buffer.
		@param[in]	maxData			Maximum number of bytes left in the given data buffer.
		@param[out]	outPacketSize	Receives the size, in bytes, of the generated packet.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GenerateTransmitData (uint8_t * pData, uint32_t maxData, uint32_t & outPacketSize);

	virtual inline const uint8_t *			GetPayloadData (void) const					{return m_pPayload;}		///< @return	A const pointer to my payload buffer.

	virtual inline size_t					GetPayloadByteCount (void) const			{return size_t (m_DC);}		///< @return	My current payload byte count.

	/**
		@brief		Streams a human-readable representation of me to the given output stream.
		@param		inOutStream		Specifies the output stream.
		@param[in]	inDetailed		Specify 'true' for a detailed representation;  otherwise use 'false' for a brief one.
		@return		The given output stream.
	**/
	virtual std::ostream &					Print (std::ostream & inOutStream, const bool inDetailed = false) const;

	/**
		@brief		Dumps a human-readable representation of my payload bytes into the given output stream.
		@return		The given output stream.
	**/
	virtual std::ostream &					DumpPayload (std::ostream & inOutStream) const;

	virtual inline std::string				IDAsString (void) const	{return DIDSIDToString (GetDID(), GetSID());}	///< @return	A string representing my DID/SID.
	virtual std::string						AsString (uint16_t inDumpMaxBytes = 0) const;	///< @return	Converts me to a compact, human-readable string.
	virtual inline AJAAncillaryDIDSIDPair	GetDIDSIDPair (void) const		{return AJAAncillaryDIDSIDPair(GetDID(),GetSID());}

	/**
		@return	A string containing a human-readable representation of the given DID/SDID values,
				or empty for invalid or unknown values.
		@param[in]	inDID	Specifies the Data ID value.
		@param[in]	inSDID	Specifies the Secondary Data ID value.
	**/
	static std::string						DIDSIDToString (const uint8_t inDID, const uint8_t inSDID);

	/**
		@param[in]	inRHS	The packet I am to be compared with.
		@return		True if I'm identical to the RHS packet.
	**/
	virtual bool							operator == (const AJAAncillaryData & inRHS) const;

	/**
		@param[in]	inRHS	The packet I am to be compared with.
		@return		True if I differ from the RHS packet.
	**/
	virtual inline bool						operator != (const AJAAncillaryData & inRHS) const		{return !(*this == inRHS);}

	protected:
		void								Init (void);	// NOT virtual - called by constructors

		AJAStatus							AllocDataMemory (const uint32_t inNumBytes);
		AJAStatus							FreeDataMemory (void);

		uint8_t								GetHeaderByte1 (void) const		{return 0xFF;}
		uint8_t								GetHeaderByte2 (void) const;
		uint8_t								GetHeaderByte3 (void) const;

	//	Instance Data
	protected:
		uint8_t						m_DID;			///< @brief	Official SMPTE ancillary packet ID (w/o parity)
		uint8_t						m_SID;			///< @brief	Official SMPTE secondary ID (or DBN - w/o parity)
		uint32_t					m_DC;			///< @brief	Number of bytes in payload (w/o parity). This may be LARGER than the traditional 255-byte ANC packet limit!
		uint8_t						m_checksum;		///< @brief	My 8-bit checksum: DID + SID + DC + payload (w/o parity) [note: NOT the same as the 9-bit checksum in a SMPTE-291 packet!]
		AJAAncillaryDataLocation	m_location;		///< @brief	Location of the ancillary data in the video stream (Y or C, HANC or VANC, etc.)
		AJAAncillaryDataCoding		m_coding;		///< @brief	Analog or digital data
		uint8_t *					m_pPayload;		///< @brief	Pointer to my payload data (DC = size)
		bool						m_rcvDataValid;	///< @brief	This is set true (or not) by ParsePayloadData()
		AJAAncillaryDataType		m_ancType;		///< @brief	One of a known set of ancillary data types (or "Custom" if not identified)

};	//	AJAAncillaryData


/**
	@brief		Writes a human-readable rendition of the given AJAAncillaryData into the given output stream.
	@param		inOutStream		Specifies the output stream to be written.
	@param[in]	inAncData		Specifies the AJAAncillaryData to be rendered into the output stream.
	@return		A non-constant reference to the specified output stream.
**/
inline std::ostream & operator << (std::ostream & inOutStream, const AJAAncillaryData & inAncData)		{return inAncData.Print (inOutStream);}

#endif	// AJA_ANCILLARYDATA_H
