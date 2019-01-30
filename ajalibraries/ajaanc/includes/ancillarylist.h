/**
	@file		ancillarylist.h
	@brief		Declares the AJAAncillaryList class.
	@copyright	(C) 2010-2019 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef AJA_ANCILLARYLIST_H
#define AJA_ANCILLARYLIST_H

#include "ancillarydata.h"
#include "ntv2formatdescriptor.h"
#include <list>
#include <map>


// used for "match any" searches and counts
const uint8_t AJAAncillaryDataWildcard_DID = 0xFF;
const uint8_t AJAAncillaryDataWildcard_SID = 0xFF;


typedef std::map <uint16_t, AJAAncillaryDataType>	AJAAncillaryAnalogTypeMap;


/**
	@brief		I am an ordered collection of AJAAncillaryData instances which represent one or more SMPTE 291
				data packets that were captured from, or destined to be played into, one video field or frame.
				I can be built from the ancillary data received by the hardware during one field/frame, and/or
				built "from scratch" and used as the source of outgoing ancillary data to hardware.

				By default, packets (::AJAAncillaryData instances) remain in the order added to me.
				Use my AJAAncillaryList::SortListByDID, AJAAncillaryList::SortListBySID or AJAAncillaryList::SortListByLocation
				methods to sort my packets by DID, SDID or location.

	@warning	I am not thread-safe! When any of my non-const methods are called by one thread, do not call any of my
				methods from any other thread.
**/
class AJAExport AJAAncillaryList
{
public:	//	CLASS METHODS
	/**
		@brief		Returns all packets found in the VANC lines of the given NTV2 frame buffer.
		@param[in]	inFrameBuffer		Specifies the NTV2 frame buffer (or the portion containing the VANC lines).
		@param[in]	inFormatDesc		Describes the frame buffer (pixel format, video standard, etc.).
		@param[out]	outPackets			Receives the packets found.
		@return		AJA_STATUS_SUCCESS if successful.
		@bug		The ::AJAAncillaryDataLink in the ::AJAAncillaryDataLocation in each of the returned packets
					is currently ::AJAAncillaryDataLink_A, which will be incorrect if, for example, the FrameStore
					that delivered the \c inFrameBuffer was sourced from the "B" link of a Dual-Link SDI source.
	**/
	static AJAStatus						SetFromVANCData (const NTV2_POINTER & inFrameBuffer,
															const NTV2FormatDescriptor & inFormatDesc,
															AJAAncillaryList & outPackets);

	/**
		@brief		Returns all ancillary data packets found in the given F1 and F2 ancillary data buffers.
		@param[in]	inF1AncBuffer		Specifies the F1 ancillary data ("GUMP") buffer.
		@param[in]	inF2AncBuffer		Specifies the F2 ancillary data ("GUMP") buffer.
		@param[out]	outPackets			Receives the packet list.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	static AJAStatus						SetFromSDIAncData (const NTV2_POINTER & inF1AncBuffer,
															const NTV2_POINTER & inF2AncBuffer,
															AJAAncillaryList & outPackets);

	/**
		@brief		Returns all ancillary data packets found in the given F1 and F2 RTP packet buffers.
		@param[in]	inF1AncBuffer		Specifies the F1 RTP packet buffer.
		@param[in]	inF2AncBuffer		Specifies the F2 RTP packet buffer.
		@param[out]	outPackets			Receives the packet list.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	static AJAStatus						SetFromIPAncData (const NTV2_POINTER & inF1AncBuffer,
															const NTV2_POINTER & inF2AncBuffer,
															AJAAncillaryList & outPackets);

public:	//	INSTANCE METHODS
	/**
		@name	Construction, Destruction, Assignment & Copying
	**/
	///@{
											AJAAncillaryList ();			///< @brief	Instantiate and initialize with a default set of values.

	virtual									~AJAAncillaryList ();			///< @brief	My destructor.

	/**
		@brief	Assignment operator -- replaces my contents with the right-hand-side value.
		@param[in]	inRHS	The value to be assigned to me.
		@return		A reference to myself.
	**/
	virtual AJAAncillaryList &				operator = (const AJAAncillaryList & inRHS);
	///@}


	/**
		@name	Fetching, Searching & Enumerating Packets
	**/
	///@{

	/**
		@brief	Answers with the number of AJAAncillaryData objects I contain (any/all types).
		@return	The number of AJAAncillaryData objects I contain.
	**/
	virtual inline uint32_t					CountAncillaryData (void) const				{return uint32_t(m_ancList.size());}

	/**
		@brief		Answers with the AJAAncillaryData object at the given index.
		@param[in]	inIndex		Specifies the zero-based index position.
		@return		The AJAAncillaryData object at the given index (or NULL if not found).
		@note		The AJAAncillaryList owns the returned object. If the list gets Cleared or deleted, the returned pointer will become invalid.
	**/
	virtual AJAAncillaryData *				GetAncillaryDataAtIndex (const uint32_t inIndex) const;

	/**
		@brief		Answers with the number of AJAAncillaryData objects having the given type.
		@param[in]	inMatchType		Specifies the AJAAncillaryDataType to match.
		@return		The number of AJAAncillaryData objects having the given type.
	**/
	virtual uint32_t						CountAncillaryDataWithType (const AJAAncillaryDataType inMatchType) const;

	/**
		@brief		Answers with the AJAAncillaryData object having the given type and index.
		@param[in]	inMatchType		Specifies the AJAAncillaryDataType to match.
		@param[in]	inIndex			Specifies the desired instance of the given type (use zero for the first one).
		@return		The AJAAncillaryData object (or NULL if not found).
		@note		The AJAAncillaryList owns the returned object. If the list gets Cleared or deleted, the returned pointer will become invalid.
	**/
	virtual AJAAncillaryData *				GetAncillaryDataWithType (const AJAAncillaryDataType inMatchType, const uint32_t inIndex = 0) const;

	/**
		@brief		Answers with the number of AncillaryData objects having the given DataID and SecondaryID.
		@param[in]	inDID	Specifies the DataID to match. Use AJAAncillaryDataWildcard_DID to match any/all DIDs.
		@param[in]	inSID	Specifies the secondary ID to match. Use AJAAncillaryDataWildcard_SID to match any/all SIDs.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual uint32_t						CountAncillaryDataWithID (const uint8_t inDID, const uint8_t inSID) const;

	/**
		@brief		Answers with the AJAAncillaryData object having the given DataID and SecondaryID, at the given index.
		@param[in]	inDID		DataID to match (use AJAAncillaryDataWildcard_DID to match "any" DID)
		@param[in]	inSID		Secondary ID to match (use AJAAncillaryDataWildcard_SID to match "any" SID)
		@param[in]	inIndex		Specifies which instance among those having the given DID and SID (use zero for the first one).
		@return		The AJAAncillaryData object having the given DID, SID and index.
		@note		The AJAAncillaryList owns the returned object. If the list gets Cleared or deleted, the returned pointer will become invalid.
	**/
	virtual AJAAncillaryData *				GetAncillaryDataWithID (const uint8_t inDID, const uint8_t inSID, const uint32_t inIndex = 0) const;
	///@}


	/**
		@name	Adding & Removing Packets
	**/
	///@{

	/**
		@brief	Removes and frees all of my AJAAncillaryData objects.
		@return	AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						Clear (void);

	/**
		@brief		Adds (appends) a copy (using AJAAncillaryData::Clone) of the AJAAncillaryData object to me.
		@param[in]	pInAncData	Specifies the AJAAncillaryData object to be copied and added to me.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						AddAncillaryData (const AJAAncillaryData * pInAncData);

	/**
		@brief		Adds a copy of the AJAAncillaryData object to me.
		@param[in]	inAncData	Specifies the AJAAncillaryData object to be copied and added to me.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual inline AJAStatus				AddAncillaryData (const AJAAncillaryData & inAncData)	{return AddAncillaryData(&inAncData);}

	/**
		@brief		Removes all copies of the AJAAncillaryData object from me.
		@note		The given AJAAncillaryData object is not freed/deleted -- it's only removed from my list.
		@param[in]	pInAncData	Specifies the AJAAncillaryData object to remove.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						RemoveAncillaryData (AJAAncillaryData * pInAncData);

	/**
		@brief		Removes all copies of the AJAAncillaryData object from me and deletes the object itself.
		@param[in]	pInAncData	Specifies the AJAAncillaryData object to remove and delete.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						DeleteAncillaryData (AJAAncillaryData * pInAncData);
	///@}


	/**
		@name	Operations
	**/
	///@{

	/**
		@brief		Sort the AncillaryDataList by DataID (DID) value.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SortListByDID (void);

	/**
		@brief		Sort the AncillaryDataList by Secondary ID (SID) value.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SortListBySID (void);

	/**
		@brief		Sort the AncillaryDataList by "location", i.e. where in the video (field, line num, HANC/VANC)
					the data came from or will be inserted to.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SortListByLocation (void);

	/**
		@brief		Compares me with another list.
		@param[in]	inIgnoreLocation	If true, don't compare each packet's AJAAncillaryDataLocation info. Defaults to true.
		@param[in]	inIgnoreChecksum	If true, don't compare each packet's checksums. Defaults to true.
		@return		AJA_STATUS_SUCCESS if equal;  otherwise AJA_STATUS_FAIL.
		@note		The sort order of each list, to be considered identical, must be the same.
	**/
	virtual AJAStatus						Compare (const AJAAncillaryList & inCompareList, const bool inIgnoreLocation = true,  const bool inIgnoreChecksum = true) const;
	virtual std::string						CompareWithInfo (const AJAAncillaryList & inCompareList, const bool inIgnoreLocation = true,  const bool inIgnoreChecksum = true) const;
	///@}


	/**
		@name	Transmit to AJA Hardware
	**/
	///@{
	/**
		@brief		Answers with the sizes of the buffers (one for field 1, one for field 2) needed to hold the anc data inserter
					"transmit" data for all of my AJAAncillaryData objects.
		@param[in]	inIsProgressive		Specify true for insertion into Progressive (transport) frames, or false for interlaced or psf.
		@param[in]	inF2StartLine		For interlaced/psf frames, specifies the line number where "field 2" begins;  otherwise ignored.
		@param[out]	outF1ByteCount		Receives the size (in bytes) of the buffer needed to hold the "Field 1" anc data.
		@param[out]	outF2ByteCount		Receives the size (in bytes) of the buffer needed to hold the "Field 2" anc data.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetAncillaryDataTransmitSize (const bool inIsProgressive, const uint32_t inF2StartLine,
																			uint32_t & outF1ByteCount, uint32_t & outF2ByteCount);


	/**
		@brief		Builds one or two ancillary data buffers (one for field 1, one for field 2) with the anc data inserter
					"transmit" data for all of my AJAAncillaryData objects.
		@param[in]	inIsProgressive		Specify true for insertion into Progressive (transport) frames, or false for interlaced or psf.
		@param[in]	inF2StartLine		For interlaced/psf frames, specifies the line number where "field 2" begins;  otherwise ignored.
		@param		pOutF1AncData		Specifies the valid, non-NULL starting address of the "Field 1" ancillary data buffer.
										Note that this buffer is written for Progressive frames.
		@param[in]	inF1ByteCountMax	Specifies the capacity (in bytes) of the Field 1 buffer (may be larger than needed).
		@param		pOutF2AncData		Specifies the valid, non-NULL starting address of the "Field 2" ancillary data buffer.
										Note that this buffer is not written for Progressive frames.
		@param[in]	inF2ByteCountMax	Specifies the capacity (in bytes) of the Field 2 buffer (may be larger than needed).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetAncillaryDataTransmitData (const bool inIsProgressive, const uint32_t inF2StartLine,
																			uint8_t * pOutF1AncData, const uint32_t inF1ByteCountMax,
																			uint8_t * pOutF2AncData, const uint32_t inF2ByteCountMax);

	/**
		@brief		Writes my AJAAncillaryData objects into the given buffers for insertion into an SDI data stream
					in \ref ancgumpformat.
		@param		F1Buffer			Specifies the buffer memory into which Field 1's anc data will be written.
		@param		F2Buffer			Specifies the buffer memory into which Field 2's anc data will be written.
		@param		inIsProgressive		Specify true to designate the output ancillary data stream as progressive; 
										otherwise, specify false. Defaults to true (is progressive).
		@param[in]	inF2StartLine		For interlaced/psf frames, specifies the line number where Field 2 begins;  otherwise ignored.
										Defaults to zero (progressive). For interlaced video, see NTV2SmpteLineNumber::GetLastLine .
		@note		It's assumed that my packets are already sorted by location.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetSDITransmitData (NTV2_POINTER & F1Buffer, NTV2_POINTER & F2Buffer,
																const bool inIsProgressive = true, const uint32_t inF2StartLine = 0);

	/**
		@brief		Writes my AJAAncillaryData objects into the given tall/taller frame buffer having the given raster/format.
		@param		inFrameBuffer		Specifies the frame buffer memory on the host to modify.
		@param[in]	inFormatDesc		Describes the frame buffer's raster and pixel format.
		@note		It's a good idea to call AJAAncillaryList::SortListByLocation before calling this function.
		@return		AJA_STATUS_SUCCESS if successful.
		@bug		Currently ignores each packet's horizontal offset (assumes AJAAncDataHorizOffset_Anywhere).
	**/
	virtual AJAStatus						GetVANCTransmitData (NTV2_POINTER & inFrameBuffer,  const NTV2FormatDescriptor & inFormatDesc);

	/**
		@brief		Writes my AJAAncillaryData objects into the given buffers as an IP/RTP data structure suitable for
					insertion into an IP ancillary data stream in \ref ancrtpformat.
		@param		F1Buffer			Specifies the buffer memory into which Field 1's IP/RTP data will be written.
		@param		F2Buffer			Specifies the buffer memory into which Field 2's IP/RTP data will be written.
		@param		inIsProgressive		Specify true to designate the output ancillary data stream as progressive; 
										otherwise, specify false. Defaults to true (is progressive).
		@param[in]	inF2StartLine		For interlaced/psf frames, specifies the line number where Field 2 begins;  otherwise ignored.
										Defaults to zero (progressive).
		@note		It's assumed that my packets are already sorted by location.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetIPTransmitData (NTV2_POINTER & F1Buffer, NTV2_POINTER & F2Buffer,
																const bool inIsProgressive = true, const uint32_t inF2StartLine = 0);
	///@}


	/**
		@name	Receive from AJA Hardware
	**/
	///@{

	/**
		@brief		Parse "raw" ancillary data bytes received from hardware (ingest) -- see \ref ancgumpformat --
					into separate AJAAncillaryData objects and appends them to me.
		@param[in]	pInReceivedData		Specifies a valid, non-NULL address of the first byte of "raw" ancillary data received by an AncExtractor widget.
		@param[in]	inByteCount			Specifies the number of bytes of data in the specified buffer to process.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						AddReceivedAncillaryData (const uint8_t * pInReceivedData, const uint32_t inByteCount);


	/**
		@brief		Parse a "raw" RTP packet received from hardware (ingest) in network byte order into separate
					AJAAncillaryData objects and appends them to me.
		@param[in]	inReceivedData		The received packet words in network byte order.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						AddReceivedAncillaryData (const std::vector<uint32_t> & inReceivedData);


	/**
		@brief		Adds the packet that originated in the VANC lines of an NTV2 frame buffer to my list.
		@param[in]	inPacketWords		Specifies the "raw" 16-bit User Data Words of the packet to be added. The first
										six elements must be 0x0000, 0x03ff, 0x03ff, DID, SDID, DC, data words, and CS.
										Each word will have its upper byte masked off.
		@param[in]	inLocation			Specifies where the packet was found.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						AddVANCData (const std::vector<uint16_t> & inPacketWords,
														const AJAAncillaryDataLocation & inLocation);


#if !defined(NTV2_DEPRECATE_14_2)
	/**
		@deprecated	Use the 2-parameter version of this function instead.
	**/
	virtual NTV2_DEPRECATED_f(AJAStatus		AddVANCData (const std::vector<uint16_t> & inPacketWords, const uint16_t inLineNum,
														const AJAAncillaryDataVideoStream inStream = AJAAncillaryDataChannel_Y));
#endif	//	!defined(NTV2_DEPRECATE_14_2)

	/**
		@brief		Sends a "ParsePayloadData" command to all of my AJAAncillaryData objects.
		@return		AJA_STATUS_SUCCESS if all items parse successfully;  otherwise the last failure result.
	**/
	virtual AJAStatus						ParseAllAncillaryData (void);
	///@}


	/**
		@name	Analog Anc Type Mapping
	**/
	///@{

//--------------------------------------------------------
	/**
	 *	The Analog Ancillary Data Type Map is a way for the user to associate certain frame line line numbers
	 *	with specific types of analog ancillary data. For example, in 525i you may wish to say, "if you find
	 *	any analog ancillary data on line 21, it is going to be AJAAncillaryDataType_Cea608_Line21. The
	 *	implementation is a std::map, and the following access methods may be used to set/get the map values.
	 *
	 *	Note: this is ONLY used by AddReceivedAncillaryData() to identify captured "analog" (AJAAncillaryDataCoding_Analog)
	 *	      data. It should be loaded BEFORE calling AddReceivedAncillaryData().
	 *
	 */

	/**
		@brief		Clears my local Analog Ancillary Data Type map.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						ClearAnalogAncillaryDataTypeMap (void);
	

	/**
		@brief		Copies the contents of <inMap> to the local Analog Ancillary Data Type map.
		@param[in]	inMap	The map to copy.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetAnalogAncillaryDataTypeMap (const AJAAncillaryAnalogTypeMap & inMap);
	

	/**
		@brief		Copies the contents of the local Analog Ancillary Data Type map to <outMap>.
		@param[out]	outMap	Receives a copy of my map.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetAnalogAncillaryDataTypeMap (AJAAncillaryAnalogTypeMap & outMap) const;


	/**
		@brief		Sets (or changes) the map entry for the designated line to the designated type.
		@note		Setting a particular line to AJAAncillaryDataType_Unknown erases the entry for that line,
					since a non-entry is treated the same as AJAAncillaryDataType_Unknown.
		@param[in]	inLineNum	Specifies the frame line number to be inserted/modified.
		@param[in]	inType		The ancillary data type to be associated with this line.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual	AJAStatus						SetAnalogAncillaryDataTypeForLine (const uint16_t inLineNum, const AJAAncillaryDataType inType);


	/**
		@brief		Answers with the ancillary data type associated with the designated line.
		@param[in]	inLineNum		Specifies the frame line number of interest.
		@return		The ancillary data type associated with the designated line, if known, or AJAAncillaryDataType_Unknown.
	**/
	virtual AJAAncillaryDataType			GetAnalogAncillaryDataTypeForLine (const uint16_t inLineNum) const;
	///@}


	/**
		@name	Printing & Debugging
	**/
	///@{

	/**
		@brief		Dumps a human-readable description of every packet in my list to the given output stream.
		@param[in]	inDetailed		If true, include some of the packet data;  otherwise omit packet data.
									Defaults to true.
		@return		The specified output stream.
	**/
	virtual std::ostream &					Print (std::ostream & inOutStream, const bool inDetailed = true) const;
	///@}


protected:
	virtual AJAAncillaryDataType			GetAnalogAncillaryDataType (AJAAncillaryData * pInAncData);


protected:
	typedef std::list <AJAAncillaryData *>			AJAAncillaryDataList;
	typedef AJAAncillaryDataList::const_iterator	AJAAncDataListConstIter;	///< @brief	Handy const iterator for iterating over members of an AJAAncillaryDataList.
	typedef AJAAncillaryDataList::iterator			AJAAncDataListIter;			///< @brief	Handy non-const iterator for iterating over members of an AJAAncillaryDataList.

	AJAAncillaryDataList		m_ancList;			///< @brief	My packet list
	AJAAncillaryAnalogTypeMap	m_analogTypeMap;	///< @brief	My "Analog Type Map" can be set by users to suggest where certain types of "analog"
													///			ancillary data are likely to be found. For example, in 525i systems, analog ancillary
													///			data captured on Line 21 are likely to be AJAAncillaryDataType_Cea608_Line21 type.
};	//	AJAAncillaryList


/**
	@brief		Writes a human-readable rendition of the given AJAAncillaryList into the given output stream.
	@param		inOutStream		Specifies the output stream to be written.
	@param[in]	inList			Specifies the AJAAncillaryList to be rendered into the output stream.
	@return		A non-constant reference to the specified output stream.
**/
inline std::ostream & operator << (std::ostream & inOutStream, const AJAAncillaryList & inList)		{return inList.Print(inOutStream);}

#endif	// AJA_ANCILLARYLIST_H
