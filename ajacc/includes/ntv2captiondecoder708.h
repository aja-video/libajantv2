/**
	@file		ntv2captiondecoder708.h
	@brief		Declares the CNTV2CaptionDecoder708 class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CEA708_DECODER_
#define __NTV2_CEA708_DECODER_

#include "ntv2caption708service.h"
#include "ntv2caption608dataqueue.h"
#include "ajaanc/includes/ancillarydata_cea708.h"

#ifdef MSWindows
	#include "windows.h"
	#include "stdio.h"
#endif

/**
	@brief	I am a CEA-708 caption decoder that can be used to decode caption data found in SMPTE 334 compliant ancillary data packets.
			There are three ways to feed me the requisite Anc data I'll need to decode captions:

			-#	If you already have the Anc data, call CNTV2CaptionDecoder708::SetSMPTE334AncData with a pointer and size.
			-#	If you have a video frame (with VANC area) in host memory and you want it parsed for a SMPTE-334 Anc packet,
				call CNTV2CaptionDecoder708::FindSMPTE334AncPacketInVideoFrame.
			-#	If you have hardware with SMPTE-334 VANC Grabber support, call CNTV2CaptionDecoder708::GetSMPTE334HardwareCaptionData
				to upload the data from hardware.
**/
class CNTV2CaptionDecoder708;
typedef AJARefPtr <CNTV2CaptionDecoder708>	CNTV2CaptionDecoder708Ptr;


const int kMaxNumCaptionChannelPacketInfo = 25;		///< @brief	The max number of CaptionChannelPackets we can deal with in a single frame
													///<		(a Caption Channel Packet can be as small as one "triplet", so whatever the max triplet count is...)

enum	//	The three "states" of a Caption Channel Packet:
{
	kCaptionChannelPacketClear,		///< @brief	"Empty" Caption Channel Packet
	kCaptionChannelPacketStarted,	///< @brief	We've started pouring data into a Caption Channel Packet but haven't yet finished
	kCaptionChannelPacketComplete	///< @brief	The Caption Channel Packet is "complete": it has all the data it's going to get
};


/**
	@brief	I am a container for a "Caption Channel Packet" -- i.e., a wrapper for a clump of CEA-708 caption data.
			By CEA-708 rules, Caption Channel Packets are independent ("asynchronous") of video frame boundaries, which
			means a single Caption Channel Packet could span more than one frame, and/or multiple Caption Channel Packets
			can be sent in a single frame. Since we tend to parse things on a frame-by-frame basis (how quaint...), this
			means that at the beginning of each frame we could have an incomplete Caption Channel Packet (i.e. one that
			was started last frame but not yet completed), and at the end of each frame we could have zero, one or more
			completed Caption Channel Packets, and zero or one incomplete Packets (Caption Channel Packets that have
			been started but not completed).
**/
typedef struct CaptionChannelPacketInfo
{
	int		ccpStatus;			///< @brief	kCaptionChannelClear/Started/Complete
	int		ccpSize;			///< @brief	The number of data words we're expecting according to the packet header (note, NOT the "packet_size_code" - this is the decoded size)
	int		ccpSequenceNum;		///< @brief	The sequence number (0, 1, 2, or 3) from the header
	int		ccpCurrentSize;		///< @brief	The number of data words actually loaded into the ccpData array

	UByte	ccpData [NTV2_CC708_MaxCaptionChannelPacketSize];	///< @brief	Raw packet data

} CaptionChannelPacketInfo, * CaptionChannelPacketInfoPtr;


/**
	@brief	I am a CEA-708 captioning decoder used primarily to obtain CEA-608 captions carried in CEA-708 anc data packets.
**/
class AJAExport CNTV2CaptionDecoder708 : public CNTV2CaptionLogConfig
{
	//	Class Methods
	public:
		/**
			@brief		Creates a new CNTV2CaptionEncoder708 instance.
			@param[out]	outDecoder	Receives the newly-created encoder instance.
			@return		True if successful; otherwise False.
		**/
		static bool			Create (CNTV2CaptionDecoder708Ptr & outDecoder);


	//	Instance Methods
	public:
		virtual				~CNTV2CaptionDecoder708 ();

		virtual void		Reset (void);

		/**
			@brief		Setsthe caption channel I'm currently focused on (i.e. that I'm currently "burning" into video).
			@param[in]	inChan	The NTV2Line21Channel of interest.
			@return		True if successful;  otherwise false.
		**/
		virtual bool		SetDisplayChannel (const NTV2Line21Channel inChan);

		/**
			@brief		Answers with the caption channel that I'm currently focused on (or that I'm currently "burning" into video).
			@return		My current NTV2Line21Channel of interest.
		**/
		virtual	inline NTV2Line21Channel	GetDisplayChannel (void) const		{return mLine21DisplayChannel;}

		/**
			@brief					Copies the given SMPTE 334 CDP data from the given ancillary data packet into my
									private CDP buffer.
			@param[in]	inPacket	Specifies the AJAAncillaryData_Cea708 packet that will supply the CDP data.
			@return					True if successful;  otherwise, false.
		**/
		virtual inline bool SetSMPTE334AncData (const AJAAncillaryData_Cea708 & inPacket)
									{return SetSMPTE334AncData(inPacket.GetPayloadData(), inPacket.GetPayloadByteCount());}
		/**
			@brief					Copies the given SMPTE 334 CDP data from the given ancillary data packet into my
									private CDP buffer.
			@param[in]	pInPacket	Specifies the AJAAncillaryData_Cea708 packet that will supply the CDP data.
			@return					True if successful;  otherwise, false.
		**/
		virtual inline bool SetSMPTE334AncData (const AJAAncillaryData_Cea708 * pInPacket)
									{return pInPacket ? SetSMPTE334AncData(pInPacket->GetPayloadData(), pInPacket->GetPayloadByteCount()) : false;}

		/**
			@brief					Copies a given SMPTE334 ancillary data packet into my private buffer for parsing.
			@param[in]	pInAncData	Specifies the host address of the start of the SMPTE334 data packet to be copied.
			@param[in]	inByteCount	Specifies the size, in bytes, of the data packet. Must be less than 256.
			@return					True if successful;  otherwise, false.
			@note		Do not use this function if the host buffer contains multiple ancillary data packets, even if one of them
						is a SMPTE334 packet. You must first locate the SMPTE334 packet in the buffer before calling this function.
		**/
		virtual bool		SetSMPTE334AncData (const UByte * pInAncData, const size_t inByteCount);

		/**
			@brief					Copies a given SMPTE334 ancillary data packet into my private buffer for parsing,
									assuming the packet data bytes are in the least significant byte of each 16-bit word in the buffer.
			@param[in]	pInAncData	Specifies the host address of the start of the SMPTE334 data packet to be copied.
			@param[in]	inWordCount	Specifies the data packet size. Must be less than 256.
			@return					True if successful;  otherwise, false.
			@note		Do not use this function if the host buffer contains multiple ancillary data packets, even if one of them
						is a SMPTE334 packet. You must first locate the SMPTE334 packet in the buffer before calling this function.
		**/
		virtual bool		SetSMPTE334AncData (const UWord * pInAncData, const size_t inWordCount);

		/**
			@brief		Returns a constant reference to my current SMPTE 334 Ancillary data packet.
			@param[out]	ppAncData		Specifies a pointer to a const UByte pointer variable that is to receive the pointer to my
										private Ancillary data packet buffer.
			@param[out]	outAncSize		Receives the size of my Ancillary data packet, in bytes.
		**/
		virtual bool		GetSMPTE334AncData (const UByte ** ppAncData, size_t & outAncSize) const;

		/**
			@brief		Parses the current frame's SMPTE 334 Ancillary packet and extracts the 608 and 708 caption data from it,
						assuming a SMPTE 334 Ancillary Packet has already been loaded into my private buffer.
						This function A) extracts any "608" (aka Line 21) data from the payload, and puts it into my mCC608FrameData
						member, and B) extracts the 708 "Caption Channel Packet" data from the Ancillary data, and puts it into my
						mCC708FrameData member, with the word count in mCC708FrameDataByteCount. Further parsing of the extracted
						608 or 708 data is left to other functions.
			@param[out]	outHasParityErrors	Specifies a boolean variable that is to receive "true" if any parity errors were detected
											while parsing the Ancillary data.
			@note		This method assumes that the parity bits have already been stripped from the loaded Anc data. If the client
						needs to perform parity checks, they'll need to be done before calling this method.
		**/
		virtual bool		ParseSMPTE334AncPacket (bool & outHasParityErrors);


		/**
			@brief		Parses the current frame's SMPTE 334 Ancillary packet and extracts the 608 and 708 caption data from it,
						assuming a SMPTE 334 Ancillary Packet has already been loaded into my private buffer.
						This function A) extracts any "608" (aka Line 21) data from the payload, and puts it into my mCC608FrameData
						member, and B) extracts the 708 "Caption Channel Packet" data from the Ancillary data, and puts it into my
						mCC708FrameData member, with the word count in mCC708FrameDataByteCount. Further parsing of the extracted
						608 or 708 data is left to other functions.
			@note		This method assumes that the parity bits have already been stripped from the loaded Anc data. If the client
						needs to perform parity checks, they'll need to be done before calling this method.
		**/
		virtual bool		ParseSMPTE334AncPacket (void);
		virtual bool		ParseSMPTE334CCTimeCodeSection (size_t & index);
		virtual bool		ParseSMPTE334CCDataSection (size_t & index);
		virtual bool		ParseSMPTE334CCServiceInfoSection (size_t & index);
		virtual bool		UpdateServiceInfo (void);

		virtual bool		Clear608CaptionData (void);

		virtual inline const CaptionData &	GetCurrentFrameCC608Data (void) const							{return mCC608FrameData;}

		/**
			@brief	Pops the next CC608 data.
			@return	The CaptionData, if any, found in my current anc packet.
		**/
		virtual CaptionData	GetCC608CaptionData (void);

		virtual bool		GetCaptionChannelPacket (UBytePtr & outDataPtr, size_t & outSize);

		virtual inline const NTV2_CC708ServiceData &	GetAllServiceInfoPtr (void) const					{return mCurrentServiceInfo.GetAllServiceInfoPtr();}

		virtual bool		GetNextServiceBlockInfoFromQueue (const size_t svcIndex, size_t & outBlockSize, size_t & outDataSize, int & outServiceNum, bool & outIsExtended) const;
		virtual size_t		GetNextServiceBlockFromQueue (const size_t svcIndex, std::vector<UByte> & outData);
		virtual size_t		GetNextServiceBlockDataFromQueue (const size_t svcIndex, std::vector<UByte> & outData);
		virtual size_t		GetNextServiceBlockFromQueue (const size_t svcIndex, UByte * pOutDataBuffer);		///< @deprecated
		virtual size_t		GetNextServiceBlockDataFromQueue (const size_t svcIndex, UByte * pOutDataBuffer);	///< @deprecated

		virtual inline void	GetCDPData (NTV2_CC708CDPPtr pCDP) const										{*pCDP = mParsedCDPData;}

		virtual void		ClearAllCaptionChannelPacketInfo (void);
		virtual void		ClearCaptionChannelPacketInfo (CaptionChannelPacketInfoPtr pCCPInfo);
		virtual int			GetNumCompleteCaptionChannelPacketInfo (int * pCompleteCount = NULL, int * pStartedCount = NULL);
		virtual void		ResetCaptionChannelPacketInfoForNewCDP (void);
		virtual void		CloseCurrentCaptionChannelPacketInfo (void);
		virtual bool		AddCaptionChannelPacketInfoData (UByte newData);
		virtual bool		GetCaptionChannelPacketInfoData (int index, UByte ** ppData, int * pSize);

		virtual bool		ParseAllCaptionChannelPackets (void);
		virtual bool		ParseCaptionChannelPacket (CaptionChannelPacketInfoPtr pCCPInfo);

		virtual bool		FindSMPTE334AncPacketInVideoFrame (const UByte * pInVideoFrame, const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inPixelFormat, bool & outHasParityErrors);
		virtual bool		FindSMPTE334AncPacketInVideoFrame (const UByte * pInVideoFrame, const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inPixelFormat);

		#if defined (_DEBUG)
			//	Debugging Methods
			virtual bool		DebugParseSMPTE334AncPacket	(bool & outHasErrors);
			virtual bool		DebugParseCDPHeader			(const UByte * pInData, size_t pktIndex, size_t maxPacketSize, size_t * pNewIndex, NTV2_CC708CDPHeaderPtr				pHdr,	bool & outHasErrors);
			virtual bool		DebugParseCDPTimecode		(const UByte * pInData, size_t pktIndex, size_t maxPacketSize, size_t * pNewIndex, NTV2_CC708CDPTimecodeSectionPtr		pTC,	bool & outHasErrors);
			virtual bool		DebugParseCDPData			(const UByte * pInData, size_t pktIndex, size_t maxPacketSize, size_t * pNewIndex, NTV2_CC708CDPDataSectionPtr			pCC,	bool & outHasErrors);
			virtual bool		DebugParseCDPServiceInfo	(const UByte * pInData, size_t pktIndex, size_t maxPacketSize, size_t * pNewIndex, NTV2_CC708CDPServiceInfoSectionPtr	pSvc,	bool & outHasErrors);
			virtual bool		DebugParseCDPFutureSection	(const UByte * pInData, size_t pktIndex, size_t maxPacketSize, size_t * pNewIndex,												bool & outHasErrors);
			virtual bool		DebugParseCDPFooter			(const UByte * pInData, size_t pktIndex, size_t maxPacketSize, size_t * pNewIndex, NTV2_CC708CDPFooterPtr				pFtr,	bool & outHasErrors);

			/**
				@brief					Logs a human-readable description of the given CEA-608 caption data.
				@param[in]	inChar1		Specifies the first caption byte. It may have its parity bit set or not.
				@param[in]	inChar2		Specifies the second caption byte. It may have its parity bit set or not.
				@param[in]	inFieldNum	Specifies the interlace field number (0 or 1).
				@return					True if something was logged; otherwise False if there was nothing worth printing.
			**/
			virtual bool		DebugParse608CaptionData			(UByte inChar1, UByte inChar2, const int inFieldNum);
			virtual bool		DebugParse708CaptionData			(const UByte * pInData, const size_t byteCount, const bool bHexDump = false) const;
			virtual size_t		DebugParse708CaptionChannelPacket	(const UByte * pInData, const size_t byteCount, size_t currIndex) const;
			virtual size_t		DebugParse708ServiceBlock			(const UByte * pInData, const size_t byteCount, size_t currIndex) const;
		//	virtual int			DebugParse708Data					(const UByte * pInData, const size_t byteCount, size_t currIndex);
			virtual inline int	GetDebugFrameNum (void) const		{return mDebugFrameNumber;}
		#endif	//	_DEBUG
	public:
		virtual NTV2CaptionLogMask	SetLogMask (const NTV2CaptionLogMask inLogMask);


	private:
		explicit								CNTV2CaptionDecoder708 ();
		explicit								CNTV2CaptionDecoder708 (const CNTV2CaptionDecoder708 & inDecoderToCopy);
		virtual CNTV2CaptionDecoder708 &		operator = (const CNTV2CaptionDecoder708 & inDecoderToCopy);

		virtual void		StartNewCaptionChannelPacketInfo (int seqNum, int expectedSize);


	//	Instance Data
	private:
		UByte							mRawVancDataBuffer [256];	///< @brief	Raw SMPTE 334 VANC data for this frame (UDW words only - no Anc header or checksum)
		size_t							mRawVancByteCount;			///< @brief	Number of bytes in mRawVancDataBuffer

		CaptionData						mCC608FrameData;			///< @brief	608 caption data for this frame

		UByte							mCC708FrameData [256];		///< @brief	708 caption channel packets for this frame
		size_t							mCC708FrameDataByteCount;	///< @brief	Number of bytes in mCC708FrameData

		// debug stuff
		int								mDebugFrameNumber;
		NTV2_CC708CDP					mParsedCDPData;				///< @brief	A parsed copy of our 708 CDP
		UWord							mPreviousSequenceCount;		///< @brief	The sequence count value for the most-recently-parsed frame in the decoder
		bool							mIsNonContiguousFrame;		///< @brief	Set true if current frame is not one count more than previous frame

		NTV2Line21Channel				mLine21DisplayChannel;		///< @brief	608 channel of interest

		CaptionChannelPacketInfo		mCCPInfoArray [kMaxNumCaptionChannelPacketInfo];	///< @brief	An array of Caption Channel Packets
		size_t							mCurrentCCPIndex;									///< @brief	Which CCP array index I'm currently filling

		//NTV2_CC708CDPTimecodeSection	mLastCDPTimecode;			///< @brief	The timecode info received in the last CDP

		//	The service_info data has to be double-buffered because new data can trickle in over the course of multiple CDPs.
		//	"mCurrentServiceInfo" is a copy of the most recent "good" data. "mNewServiceInfo" is used to accumulate the new service_info
		//	data as it comes in. When a full set has been successfully received, it is copied into mCurrentServiceInfo".
		CNTV2Caption708ServiceInfo		mCurrentServiceInfo;		///< @brief	Holds the current "official" service info state
		CNTV2Caption708ServiceInfo		mNewServiceInfo;			///< @brief	Accumulates the input service info data until it has all been received

		CNTV2Caption708Service			mAvailableServices [NTV2_CC708MaxNumServices];	///< @brief	Array of available services

		CNTV2Caption608DataQueue		mCC608QueueField1;			///< @brief	Queue for received "Field 1" 608 data
		CNTV2Caption608DataQueue		mCC608QueueField2;			///< @brief	Queue for received "Field 2" 608 data

		int								mDebugPrintOffset;			///< @brief	Offset added to debug levels (used in cases where there are multiple instances,
																	///<		e.g. "input" vs. "output"
};	//	CNTV2CaptionDecoder708

#endif	// __NTV2_CEA708_DECODER_
